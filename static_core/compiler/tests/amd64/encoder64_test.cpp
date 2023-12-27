/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <climits>

#include <random>
#include <type_traits>
#include <gtest/gtest.h>

#include "macros.h"
#include "mem/code_allocator.h"
#include "mem/pool_manager.h"
#include "target/amd64/target.h"
#include "mem/base_mem_stats.h"

template <typename T>
const char *TypeName();
template <typename T>
const char *TypeName(T /*unused*/)
{
    return TypeName<T>();
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CLASS_NAME(type)         \
    template <>                  \
    const char *TypeName<type>() \
    {                            \
        return #type;            \
    }

CLASS_NAME(int8_t)
CLASS_NAME(int16_t)
CLASS_NAME(int32_t)
CLASS_NAME(int64_t)
CLASS_NAME(uint8_t)
CLASS_NAME(uint16_t)
CLASS_NAME(uint32_t)
CLASS_NAME(uint64_t)
CLASS_NAME(float)
CLASS_NAME(double)

const uint64_t SEED = 0x1234;
#ifndef PANDA_NIGHTLY_TEST_ON
const uint64_t ITERATION = 40;
#else
const uint64_t ITERATION = 4000;
#endif
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects,cert-msc51-cpp)
static inline auto g_randomGenerator = std::mt19937_64(SEED);

// Max and min exponent on the basus of two float and double
static const float MIN_EXP_BASE2_FLOAT = std::log2(FLT_MIN);
static const float MAX_EXP_BASE2_FLOAT = std::log2(FLT_MAX) - 1.0;
static const double MIN_EXP_BASE2_DOUBLE = std::log2(DBL_MIN);
static const double MAX_EXP_BASE2_DOUBLE = std::log2(DBL_MAX) - 1.0;

// Masks for generete denormal float numbers
static const uint32_t MASK_DENORMAL_FLOAT = 0x807FFFFF;
static const uint64_t MASK_DENORMAL_DOUBLE = 0x800FFFFFFFFFFFFF;

// NOLINTBEGIN(readability-magic-numbers)
template <typename T>
static T RandomGen()
{
    auto gen {g_randomGenerator()};

    if constexpr (std::is_integral_v<T>) {
        return gen;
    } else {
        switch (gen % 20U) {
            case (0U):
                return std::numeric_limits<T>::quiet_NaN();

            case (1U):
                return std::numeric_limits<T>::infinity();

            case (2U):
                return -std::numeric_limits<T>::infinity();

            case (3U): {
                if constexpr (std::is_same_v<T, float>) {
                    return panda::bit_cast<float, uint32_t>(gen & MASK_DENORMAL_FLOAT);
                } else {
                    return panda::bit_cast<double, uint64_t>(gen & MASK_DENORMAL_DOUBLE);
                }
            }
            default:
                break;
        }

        // Uniform distribution floating value
        std::uniform_real_distribution<T> disNum(1.0, 2.0);
        int8_t sign = (gen % 2) == 0 ? 1 : -1;
        if constexpr (std::is_same_v<T, float>) {
            std::uniform_real_distribution<float> dis(MIN_EXP_BASE2_FLOAT, MAX_EXP_BASE2_FLOAT);
            return sign * disNum(g_randomGenerator) * std::pow(2.0F, dis(g_randomGenerator));
        } else if constexpr (std::is_same_v<T, double>) {
            std::uniform_real_distribution<double> dis(MIN_EXP_BASE2_DOUBLE, MAX_EXP_BASE2_DOUBLE);
            return sign * disNum(g_randomGenerator) * std::pow(2.0, dis(g_randomGenerator));
        }

        UNREACHABLE();
    }
}

namespace panda::compiler {
class Encoder64Test : public ::testing::Test {
public:
    Encoder64Test()
    {
        panda::mem::MemConfig::Initialize(64_MB, 64_MB, 64_MB, 32_MB, 0, 0);
        PoolManager::Initialize();
        allocator_ = new ArenaAllocator(SpaceType::SPACE_TYPE_COMPILER);
        encoder_ = Encoder::Create(allocator_, Arch::X86_64, false);
        encoder_->InitMasm();
        regfile_ = RegistersDescription::Create(allocator_, Arch::X86_64);
        callconv_ = CallingConvention::Create(allocator_, encoder_, regfile_, Arch::X86_64);
        encoder_->SetRegfile(regfile_);
        memStats_ = new BaseMemStats();
        codeAlloc_ = new (std::nothrow) CodeAllocator(memStats_);
    }
    ~Encoder64Test() override
    {
        Logger::Destroy();
        encoder_->~Encoder();
        delete allocator_;
        delete codeAlloc_;
        delete memStats_;
        PoolManager::Finalize();
        panda::mem::MemConfig::Finalize();
    }

    NO_COPY_SEMANTIC(Encoder64Test);
    NO_MOVE_SEMANTIC(Encoder64Test);

    void Reset()
    {
        encoder_->~Encoder();
        encoder_ = Encoder::Create(allocator_, Arch::X86_64, false);
        encoder_->InitMasm();
    }

    CodeAllocator *GetCodeAllocator()
    {
        return codeAlloc_;
    }

    void ResetCodeAllocator(void *ptr, size_t size)
    {
        os::mem::MapRange<std::byte> memRange(static_cast<std::byte *>(ptr), size);
        memRange.MakeReadWrite();
        delete codeAlloc_;
        codeAlloc_ = new (std::nothrow) CodeAllocator(memStats_);
    }

    ArenaAllocator *GetAllocator()
    {
        return allocator_;
    }

    Encoder *GetEncoder()
    {
        return encoder_;
    }

    RegistersDescription *GetRegfile()
    {
        return regfile_;
    }

    CallingConvention *GetCallconv()
    {
        return callconv_;
    }

    size_t GetCursor()
    {
        return currCursor_;
    }

    // Warning! Do not use multiply times with different types!
    Reg GetParameter(TypeInfo type, int id = 0)
    {
        ASSERT(id < 4);
        if (type.IsFloat()) {
            return Reg(id, type);
        }
        return Target::Current().GetParamReg(id, type);
    }

    void PreWork()
    {
        // Curor need to encode multiply tests due one execution
        currCursor_ = 0;
        encoder_->SetCursorOffset(0);
        callconv_->GeneratePrologue(FrameInfo::FullPrologue());
    }

    template <typename T>
    void PostWork()
    {
        if constexpr (std::is_integral_v<T>) {
            auto param = Target::Current().GetParamReg(0);
            auto returnReg = Target::Current().GetReturnReg();
            GetEncoder()->EncodeMov(returnReg, param);
        }

        callconv_->GenerateEpilogue(FrameInfo::FullPrologue(), []() {});
        encoder_->Finalize();
    }

    void Dump(bool enabled)
    {
        if (enabled) {
            auto size = callconv_->GetCodeSize() - currCursor_;
            for (uint32_t i = currCursor_; i < currCursor_ + size;) {
                i = encoder_->DisasmInstr(std::cout, i, 0);
                std::cout << std::endl;
            }
        }
    }

    template <typename T, typename U>
    bool CallCode(const T &param, const U &result)
    {
        // Using max size type: type result "U" or 32bit to check result,
        // because in our ISA min type is 32bit.
        // Only integers less thаn 32bit.
        using UExp = typename std::conditional<(sizeof(U) * BYTE_SIZE) >= WORD_SIZE, U, uint32_t>::type;
        using TExp = typename std::conditional<(sizeof(T) * BYTE_SIZE) >= WORD_SIZE, T, uint32_t>::type;
        using FunctPtr = UExp (*)(TExp data);
        auto size = callconv_->GetCodeSize() - currCursor_;
        void *offset = (static_cast<uint8_t *>(callconv_->GetCodeEntry()));
        void *ptr = codeAlloc_->AllocateCode(size, offset);
        auto func = reinterpret_cast<FunctPtr>(ptr);
        // Extend types less then 32bit (i8, i16)
        const UExp currResult = func(static_cast<TExp>(param));
        ResetCodeAllocator(ptr, size);
        bool ret = false;
        if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
            ret = (currResult == result && std::signbit(currResult) == std::signbit(result)) ||
                  (std::isnan(currResult) && std::isnan(result));
        } else {
            ret = (currResult - result == 0);
        }
        if (!ret) {
            std::cerr << std::hex << "Failed CallCode for param=" << param << " and result=" << result
                      << " current_reslt=" << currResult << "\n";
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
                std::cerr << "In binary :";
                if constexpr (std::is_same<double, T>::value) {
                    std::cerr << " param=" << bit_cast<uint64_t>(param);
                } else if constexpr (std::is_same<float, T>::value) {
                    std::cerr << " param=" << bit_cast<uint32_t>(param);
                }
                if constexpr (std::is_same<double, U>::value) {
                    std::cerr << " reslt=" << bit_cast<uint64_t>(result);
                    std::cerr << " current_reslt=" << bit_cast<uint64_t>(currResult);
                } else if constexpr (std::is_same<float, U>::value) {
                    std::cerr << " result=" << bit_cast<uint32_t>(result);
                    std::cerr << " current_reslt=" << bit_cast<uint32_t>(currResult);
                }
                std::cerr << "\n";
            }
            Dump(true);
        }
        return ret;
    }

    template <typename T, typename U>
    bool CallCode(const T &param1, const T &param2, const U &result)
    {
        using UExp = typename std::conditional<(sizeof(U) * BYTE_SIZE) >= WORD_SIZE, U, uint32_t>::type;
        using TExp = typename std::conditional<(sizeof(T) * BYTE_SIZE) >= WORD_SIZE, T, uint32_t>::type;
        using FunctPtr = UExp (*)(TExp param1, TExp param2);
        auto size = callconv_->GetCodeSize() - currCursor_;
        void *offset = (static_cast<uint8_t *>(callconv_->GetCodeEntry()));
        void *ptr = codeAlloc_->AllocateCode(size, offset);
        auto func = reinterpret_cast<FunctPtr>(ptr);
        const UExp currResult = func(static_cast<TExp>(param1), static_cast<TExp>(param2));
        ResetCodeAllocator(ptr, size);
        bool ret = false;
        if constexpr (std::is_same<float, T>::value || std::is_same<double, T>::value) {
            ret = (currResult == result && std::signbit(currResult) == std::signbit(result)) ||
                  (std::isnan(currResult) && std::isnan(result));
        } else {
            ret = (currResult - result == 0);
        }
        if (!ret) {
            std::cerr << "Failed CallCode for param1=" << param1 << " param2=" << param2 << " and result=" << result
                      << " current_result=" << currResult << "\n";
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
                std::cerr << "In binary :";
                if constexpr (std::is_same<double, T>::value) {
                    std::cerr << " param1=" << bit_cast<uint64_t>(param1) << " param2=" << bit_cast<uint64_t>(param2);
                } else if constexpr (std::is_same<float, T>::value) {
                    std::cerr << " param1=" << bit_cast<uint32_t>(param1) << " param2=" << bit_cast<uint32_t>(param2);
                }
                if constexpr (std::is_same<double, U>::value) {
                    std::cerr << " reslt=" << bit_cast<uint64_t>(result);
                    std::cerr << " current_reslt=" << bit_cast<uint64_t>(currResult);
                } else if constexpr (std::is_same<float, U>::value) {
                    std::cerr << " result=" << bit_cast<uint32_t>(result);
                    std::cerr << " current_result=" << bit_cast<uint32_t>(currResult);
                }
                std::cerr << "\n";
            }
            Dump(true);
        }
        return ret;
    }

    template <typename T>
    bool CallCode(const T &param, const T &result)
    {
        using TExp = typename std::conditional<(sizeof(T) * BYTE_SIZE) >= WORD_SIZE, T, uint32_t>::type;
        using FunctPtr = TExp (*)(TExp data);
        auto size = callconv_->GetCodeSize() - currCursor_;
        void *offset = (static_cast<uint8_t *>(callconv_->GetCodeEntry()));
        void *ptr = codeAlloc_->AllocateCode(size, offset);
        auto func = reinterpret_cast<FunctPtr>(ptr);
        const TExp currResult = func(static_cast<TExp>(param));
        ResetCodeAllocator(ptr, size);
        bool ret = false;
        if constexpr (std::is_same<float, T>::value || std::is_same<double, T>::value) {
            ret = (currResult == result && std::signbit(currResult) == std::signbit(result)) ||
                  (std::isnan(currResult) && std::isnan(result));
        } else {
            ret = (currResult - result == 0);
        }
        if (!ret) {
            std::cerr << std::hex << "Failed CallCode for param=" << param << " and result=" << result
                      << " current_result=" << currResult << "\n";
            if constexpr (std::is_floating_point_v<T>) {
                std::cerr << "In binary :";
                if constexpr (std::is_same<double, T>::value) {
                    std::cerr << " param=" << bit_cast<uint64_t>(param);
                    std::cerr << " reslt=" << bit_cast<uint64_t>(result);
                    std::cerr << " curr_reslt=" << bit_cast<uint64_t>(currResult);
                } else if constexpr (std::is_same<float, T>::value) {
                    std::cerr << " param=" << bit_cast<uint32_t>(param);
                    std::cerr << " curr_result=" << bit_cast<uint32_t>(currResult);
                }
                std::cerr << "\n";
            }
            Dump(true);
        }
        return ret;
    }

    template <typename T>
    bool CallCode(const T &param1, const T &param2, const T &result)
    {
        using TExp = typename std::conditional<(sizeof(T) * BYTE_SIZE) >= WORD_SIZE, T, uint32_t>::type;
        using FunctPtr = TExp (*)(TExp param1, TExp param2);
        auto size = callconv_->GetCodeSize() - currCursor_;
        void *offset = (static_cast<uint8_t *>(callconv_->GetCodeEntry()));
        void *ptr = codeAlloc_->AllocateCode(size, offset);
        auto func = reinterpret_cast<FunctPtr>(ptr);
        const TExp currResult = func(static_cast<TExp>(param1), static_cast<TExp>(param2));
        ResetCodeAllocator(ptr, size);
        bool ret = false;
        if constexpr (std::is_same<float, T>::value || std::is_same<double, T>::value) {
            ret = (currResult == result && std::signbit(currResult) == std::signbit(result)) ||
                  (std::isnan(currResult) && std::isnan(result));
        } else {
            ret = (currResult - result == 0);
        }
        if (!ret) {
            std::cerr << "Failed CallCode for param1=" << param1 << " param2=" << param2 << " and result=" << result
                      << " curr_result=" << currResult << "\n";
            if constexpr (std::is_floating_point_v<T>) {
                std::cerr << "In binary :";
                if constexpr (std::is_same<double, T>::value) {
                    std::cerr << " param1=" << bit_cast<uint64_t>(param1) << " param2=" << bit_cast<uint64_t>(param2);
                    std::cerr << " reslt=" << bit_cast<uint64_t>(result)
                              << " curr_result=" << bit_cast<uint64_t>(currResult);
                } else if constexpr (std::is_same<float, T>::value) {
                    std::cerr << " param1=" << bit_cast<uint32_t>(param1) << " param2=" << bit_cast<uint32_t>(param2);
                    std::cerr << " result=" << bit_cast<uint32_t>(result)
                              << " curr_result=" << bit_cast<uint32_t>(currResult);
                }
                std::cerr << "\n";
            }
            Dump(true);
        }
        return ret;
    }

    template <typename T>
    T CallCodeStore(uint64_t address, T param)
    {
        using FunctPtr = T (*)(uint64_t param1, T param2);
        auto size = callconv_->GetCodeSize() - currCursor_;
        void *offset = (static_cast<uint8_t *>(callconv_->GetCodeEntry()));
        void *ptr = codeAlloc_->AllocateCode(size, offset);
        auto func = reinterpret_cast<FunctPtr>(ptr);
        const T currResult = func(address, param);
        ResetCodeAllocator(ptr, size);
        return currResult;
    }

    template <typename T, typename U>
    U CallCodeCall(T param1, T param2)
    {
        using FunctPtr = U (*)(T param1, T param2);
        auto size = callconv_->GetCodeSize() - currCursor_;
        void *offset = (static_cast<uint8_t *>(callconv_->GetCodeEntry()));
        void *ptr = codeAlloc_->AllocateCode(size, offset);
        auto func = reinterpret_cast<FunctPtr>(ptr);
        const U currResult = func(param1, param2);
        ResetCodeAllocator(ptr, size);
        return currResult;
    }

private:
    ArenaAllocator *allocator_ {nullptr};
    Encoder *encoder_ {nullptr};
    RegistersDescription *regfile_ {nullptr};
    CallingConvention *callconv_ {nullptr};
    CodeAllocator *codeAlloc_ {nullptr};
    BaseMemStats *memStats_ {nullptr};
    size_t currCursor_ {0};
};

template <typename T>
bool TestNeg(Encoder64Test *test)
{
    // Initialize
    test->PreWork();
    // First type-dependency
    auto param = test->GetParameter(TypeInfo(T(0)));
    // Main test call
    test->GetEncoder()->EncodeNeg(param, param);
    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp = RandomGen<T>();
        // Deduced conflicting types for parameter
        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp, -tmp)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T>(nan, nan)) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, NegTest)
{
    EXPECT_TRUE(TestNeg<int32_t>(this));
    EXPECT_TRUE(TestNeg<int64_t>(this));
    EXPECT_TRUE(TestNeg<float>(this));
    EXPECT_TRUE(TestNeg<double>(this));
}

template <typename T>
bool TestNot(Encoder64Test *test)
{
    // Initialize
    test->PreWork();
    // First type-dependency
    auto param = test->GetParameter(TypeInfo(T(0)));
    // Main test call
    test->GetEncoder()->EncodeNot(param, param);
    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp = RandomGen<T>();
        // Deduced conflicting types for parameter
        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp, ~tmp)) {  // NOLINT(hicpp-signed-bitwise)
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, NotTest)
{
    EXPECT_TRUE(TestNot<int32_t>(this));
    EXPECT_TRUE(TestNot<int64_t>(this));
    EXPECT_TRUE(TestNot<uint32_t>(this));
    EXPECT_TRUE(TestNot<uint64_t>(this));
}

template <typename T>
bool TestMov(Encoder64Test *test)
{
    // Initialize
    test->PreWork();
    // First type-dependency
    auto param = test->GetParameter(TypeInfo(T(0)));
    // Main test call
    test->GetEncoder()->EncodeMov(param, param);
    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp = RandomGen<T>();
        // Deduced conflicting types for parameter
        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp, tmp)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T>(nan, nan)) {
            return false;
        }
    }

    return true;
}

template <typename Src, typename Dst>
bool TestMov2(Encoder64Test *test)
{
    static_assert(sizeof(Src) == sizeof(Dst));
    // Initialize
    test->PreWork();
    // First type-dependency
    auto input = test->GetParameter(TypeInfo(Src(0)), 0);
    auto output = test->GetParameter(TypeInfo(Dst(0)), 0);
    // Main test call
    test->GetEncoder()->EncodeMov(output, input);
    // Finalize
    test->PostWork<Dst>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<Src>() << ", " << TypeName<Dst>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        Src src = RandomGen<Src>();
        Dst dst = bit_cast<Dst>(src);
        // Deduced conflicting types for parameter
        // Main check - compare parameter and
        // return value
        if (!test->CallCode<Src, Dst>(src, dst)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<Src>) {
        Src nan = std::numeric_limits<Src>::quiet_NaN();
        Dst dstNan = bit_cast<Dst>(nan);
        if (!test->CallCode<Src, Dst>(nan, dstNan)) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, MovTest)
{
    EXPECT_TRUE(TestMov<int32_t>(this));
    EXPECT_TRUE(TestMov<int64_t>(this));
    EXPECT_TRUE(TestMov<float>(this));
    EXPECT_TRUE(TestMov<double>(this));

    EXPECT_TRUE((TestMov2<float, int32_t>(this)));
    EXPECT_TRUE((TestMov2<double, int64_t>(this)));
    EXPECT_TRUE((TestMov2<int32_t, float>(this)));
    EXPECT_TRUE((TestMov2<int64_t, double>(this)));
    // NOTE (igorban) : add MOVI instructions
    // & support uint64_t mov
}

// Jump w/o cc
TEST_F(Encoder64Test, JumpTest)
{
    // Test for EncodeJump(LabelHolder::LabelId label)
    PreWork();

    auto param = Target::Current().GetParamReg(0);

    auto t1 = GetEncoder()->CreateLabel();
    auto t2 = GetEncoder()->CreateLabel();
    auto t3 = GetEncoder()->CreateLabel();
    auto t4 = GetEncoder()->CreateLabel();
    auto t5 = GetEncoder()->CreateLabel();

    GetEncoder()->EncodeAdd(param, param, Imm(0x1));
    GetEncoder()->EncodeJump(t1);
    GetEncoder()->EncodeMov(param, Imm(0x0));
    GetEncoder()->EncodeReturn();
    // T4
    GetEncoder()->BindLabel(t4);
    GetEncoder()->EncodeAdd(param, param, Imm(0x1));
    GetEncoder()->EncodeJump(t5);
    // Fail value
    GetEncoder()->EncodeMov(param, Imm(0x0));
    GetEncoder()->EncodeReturn();

    // T2
    GetEncoder()->BindLabel(t2);
    GetEncoder()->EncodeAdd(param, param, Imm(0x1));
    GetEncoder()->EncodeJump(t3);
    // Fail value
    GetEncoder()->EncodeMov(param, Imm(0x0));
    GetEncoder()->EncodeReturn();
    // T3
    GetEncoder()->BindLabel(t3);
    GetEncoder()->EncodeAdd(param, param, Imm(0x1));
    GetEncoder()->EncodeJump(t4);
    // Fail value
    GetEncoder()->EncodeMov(param, Imm(0x0));
    GetEncoder()->EncodeReturn();
    // T1
    GetEncoder()->BindLabel(t1);
    GetEncoder()->EncodeAdd(param, param, Imm(0x1));
    GetEncoder()->EncodeJump(t2);
    // Fail value
    GetEncoder()->EncodeMov(param, Imm(0x0));
    GetEncoder()->EncodeReturn();
    // Success exit
    GetEncoder()->BindLabel(t5);
    PostWork<int>();

    if (!GetEncoder()->GetResult()) {
        std::cerr << "Unsupported \n";
        return;
    }
    Dump(false);
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        auto tmp = RandomGen<int32_t>();
        // Deduced conflicting types for parameter
        EXPECT_TRUE(CallCode<int32_t>(tmp, tmp + 5));
    }
}

template <typename T, bool NOT_ZERO = false>
bool TestBitTestAndBranch(Encoder64Test *test, T value, int pos, uint32_t expected)
{
    test->PreWork();
    auto param = test->GetParameter(TypeInfo(T(0)));
    auto retVal = Target::Current().GetParamReg(0);
    auto label = test->GetEncoder()->CreateLabel();
    auto end = test->GetEncoder()->CreateLabel();

    if (NOT_ZERO) {
        test->GetEncoder()->EncodeBitTestAndBranch(label, param, pos, true);
    } else {
        test->GetEncoder()->EncodeBitTestAndBranch(label, param, pos, false);
    }
    test->GetEncoder()->EncodeMov(retVal, Imm(1));
    test->GetEncoder()->EncodeJump(end);

    test->GetEncoder()->BindLabel(label);
    test->GetEncoder()->EncodeMov(retVal, Imm(0));
    test->GetEncoder()->BindLabel(end);

    test->PostWork<T>();

    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << std::endl;
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    if (!test->CallCode<T>(value, expected)) {
        std::cerr << "Bit " << pos << " of 0x" << std::hex << value << " is not equal to " << std::dec << expected
                  << std::endl;
        return false;
    }

    return true;
}

template <typename T, bool NOT_ZERO = false>
bool TestBitTestAndBranch(Encoder64Test *test)
{
    size_t maxPos = std::is_same<uint64_t, T>::value ? 64 : 32;
    for (size_t i = 0; i < maxPos; i++) {
        T value = static_cast<T>(1) << i;
        if (!TestBitTestAndBranch<T, NOT_ZERO>(test, value, i, NOT_ZERO ? 0 : 1)) {
            return false;
        }
        if (!TestBitTestAndBranch<T, NOT_ZERO>(test, ~value, i, NOT_ZERO ? 1 : 0)) {
            return false;
        }
    }
    return true;
}

template <typename T, bool IS_EQUAL = true>
bool TestAddOverflow(Encoder64Test *test, T value1, T value2, T expected)
{
    test->PreWork();
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);
    auto retVal = Target::Current().GetParamReg(0, TypeInfo(T(0)));
    auto label = test->GetEncoder()->CreateLabel();
    auto end = test->GetEncoder()->CreateLabel();

    if (IS_EQUAL) {
        test->GetEncoder()->EncodeAddOverflow(label, retVal, param1, param2, Condition::VS);
        test->GetEncoder()->EncodeJump(end);
        test->GetEncoder()->BindLabel(label);
        test->GetEncoder()->EncodeMov(retVal, Imm(0));
    } else {
        test->GetEncoder()->EncodeAddOverflow(end, retVal, param1, param2, Condition::VC);
        test->GetEncoder()->EncodeMov(retVal, Imm(0));
    }

    test->GetEncoder()->BindLabel(end);

    test->PostWork<T>();

    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << std::endl;
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    if (!test->CallCode<T>(value1, value2, expected)) {
        std::cerr << "AddOverflow "
                  << " of 0x" << std::hex << value1 << " and 0x" << std::hex << value2 << " is not equal to "
                  << std::dec << expected << std::endl;
        return false;
    }

    return true;
}

template <typename T>
bool TestAddOverflow(Encoder64Test *test)
{
    T min = std::numeric_limits<T>::min();
    T max = std::numeric_limits<T>::max();
    std::array<T, 7> values = {0, 2, 5, -7, -10, max, min};
    for (uint32_t i = 0; i < values.size(); ++i) {
        for (uint32_t j = 0; j < values.size(); ++j) {
            T a0 = values[i];
            T a1 = values[j];
            T expected;
            if ((a0 > 0 && a1 > max - a0) || (a0 < 0 && a1 < min - a0)) {
                expected = 0;
            } else {
                expected = a0 + a1;
            }
            if (!TestAddOverflow<T, true>(test, a0, a1, expected) ||
                !TestAddOverflow<T, false>(test, a0, a1, expected)) {
                return false;
            }
        }
    }
    return true;
}

template <typename T, bool IS_EQUAL = true>
bool TestSubOverflow(Encoder64Test *test, T value1, T value2, T expected)
{
    test->PreWork();
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);
    auto retVal = Target::Current().GetParamReg(0, TypeInfo(T(0)));
    auto label = test->GetEncoder()->CreateLabel();
    auto end = test->GetEncoder()->CreateLabel();

    if (IS_EQUAL) {
        test->GetEncoder()->EncodeSubOverflow(label, retVal, param1, param2, Condition::VS);
        test->GetEncoder()->EncodeJump(end);
        test->GetEncoder()->BindLabel(label);
        test->GetEncoder()->EncodeMov(retVal, Imm(0));
    } else {
        test->GetEncoder()->EncodeSubOverflow(end, retVal, param1, param2, Condition::VC);
        test->GetEncoder()->EncodeMov(retVal, Imm(0));
    }

    test->GetEncoder()->BindLabel(end);

    test->PostWork<T>();

    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << std::endl;
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    if (!test->CallCode<T>(value1, value2, expected)) {
        std::cerr << "SubOverflow "
                  << " of 0x" << std::hex << value1 << " and 0x" << std::hex << value2 << " is not equal to "
                  << std::dec << expected << std::endl;
        return false;
    }

    return true;
}

template <typename T>
bool TestSubOverflow(Encoder64Test *test)
{
    T min = std::numeric_limits<T>::min();
    T max = std::numeric_limits<T>::max();
    std::array<T, 7> values = {0, 2, 5, -7, -10, max, min};
    for (uint32_t i = 0; i < values.size(); ++i) {
        for (uint32_t j = 0; j < values.size(); ++j) {
            T a0 = values[i];
            T a1 = values[j];
            T expected;
            if ((a1 > 0 && a0 < min + a1) || (a1 < 0 && a0 > max + a1)) {
                expected = 0;
            } else {
                expected = a0 - a1;
            }
            if (!TestSubOverflow<T, true>(test, a0, a1, expected) ||
                !TestSubOverflow<T, false>(test, a0, a1, expected)) {
                return false;
            }
        }
    }
    return true;
}

template <typename T, Condition CC>
bool TestJumpCCMainLoop(Encoder64Test *test)
{
    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp = RandomGen<T>();
        if (tmp == 0) {  // Only non-zero values
            tmp += 1;
        }
        // Deduced conflicting types for parameter

        if constexpr (CC == Condition::EQ) {
            if (!test->CallCode<T>(tmp, 1)) {
                std::cerr << "non-zero EQ test fail " << tmp << " \n";
                return false;
            }
        }
        if constexpr (CC == Condition::NE) {
            if (!test->CallCode<T>(tmp, 0)) {
                std::cerr << "non-zero EQ test fail " << tmp << " \n";
                return false;
            }
        }
        // Main check - compare parameter and
        // return value
    }
    return true;
}

template <typename T, Condition CC>
bool TestJumpCC(Encoder64Test *test)
{
    bool isSigned = std::is_signed<T>::value;
    // Initialize
    test->PreWork();
    // First type-dependency
    auto param = test->GetParameter(TypeInfo(T(0)));
    // Main test call
    auto retVal = Target::Current().GetParamReg(0);

    auto tsucc = test->GetEncoder()->CreateLabel();
    auto end = test->GetEncoder()->CreateLabel();

    test->GetEncoder()->EncodeJump(tsucc, param, CC);
    test->GetEncoder()->EncodeMov(param, Imm(0x0));
    test->GetEncoder()->EncodeMov(retVal, Imm(0x1));
    test->GetEncoder()->EncodeJump(end);

    test->GetEncoder()->BindLabel(tsucc);
    test->GetEncoder()->EncodeMov(param, Imm(0x0));
    test->GetEncoder()->EncodeMov(retVal, Imm(0x0));
    test->GetEncoder()->BindLabel(end);
    // test->GetEncoder()->EncodeReturn(); < encoded in PostWork

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    if constexpr (CC == Condition::EQ) {
        if (!test->CallCode<T>(0, 0)) {
            std::cerr << "zero EQ test fail \n";
            return false;
        }
    }
    if constexpr (CC == Condition::NE) {
        if (!test->CallCode<T>(0, 1)) {
            std::cerr << "zero EQ test fail \n";
            return false;
        }
    }
    return TestJumpCCMainLoop<T, CC>(test);
}

// Jump with cc
TEST_F(Encoder64Test, JumpCCTest)
{
    EXPECT_TRUE((TestJumpCC<int32_t, Condition::EQ>(this)));
    EXPECT_TRUE((TestJumpCC<int32_t, Condition::NE>(this)));
    EXPECT_TRUE((TestJumpCC<int64_t, Condition::NE>(this)));
    EXPECT_TRUE((TestJumpCC<int64_t, Condition::EQ>(this)));
    EXPECT_TRUE((TestJumpCC<uint32_t, Condition::EQ>(this)));
    EXPECT_TRUE((TestJumpCC<uint32_t, Condition::NE>(this)));
    EXPECT_TRUE((TestJumpCC<uint64_t, Condition::EQ>(this)));
    EXPECT_TRUE((TestJumpCC<uint64_t, Condition::NE>(this)));
    // TestJumpCC<float, Condition::EQ>
    // TestJumpCC<float, Condition::NE>
    // TestJumpCC<double, Condition::EQ>
    // TestJumpCC<double, Condition::NE>
}

TEST_F(Encoder64Test, BitTestAndBranchZero)
{
    EXPECT_TRUE(TestBitTestAndBranch<uint32_t>(this));
    EXPECT_TRUE(TestBitTestAndBranch<uint64_t>(this));
}

TEST_F(Encoder64Test, BitTestAndBranchNotZero)
{
    constexpr bool NOT_ZERO = true;
    EXPECT_TRUE((TestBitTestAndBranch<uint32_t, NOT_ZERO>(this)));
    EXPECT_TRUE((TestBitTestAndBranch<uint64_t, NOT_ZERO>(this)));
}

TEST_F(Encoder64Test, AddOverflow)
{
    EXPECT_TRUE((TestAddOverflow<int32_t>(this)));
    EXPECT_TRUE((TestAddOverflow<int64_t>(this)));
}

TEST_F(Encoder64Test, SubOverflow)
{
    EXPECT_TRUE((TestSubOverflow<int32_t>(this)));
    EXPECT_TRUE((TestSubOverflow<int64_t>(this)));
}

template <typename T>
bool TestLdr(Encoder64Test *test)
{
    bool isSigned = std::is_signed<T>::value;
    // Initialize
    test->PreWork();
    // First type-dependency
    // Some strange code - default parameter is addres (32 bit)

    auto param = Target::Current().GetParamReg(0);
    // But return value is cutted by loaded value
    auto retVal = test->GetParameter(TypeInfo(T(0)));

    auto mem = MemRef(param);
    test->GetEncoder()->EncodeLdr(retVal, isSigned, mem);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp = RandomGen<T>();
        T *ptr = &tmp;
        // Test : param - Pointer to value
        //        return - value (loaded by ptr)
        // Value is resulting type, but call is ptr_type
        if (!test->CallCode<int64_t, T>(reinterpret_cast<int64_t>(ptr), tmp)) {
            std::cerr << "Ldr test fail " << tmp << " \n";
            return false;
        }
    }
    return true;
}

// Load test
TEST_F(Encoder64Test, LoadTest)
{
    EXPECT_TRUE((TestLdr<int32_t>(this)));
    EXPECT_TRUE((TestLdr<int64_t>(this)));
    EXPECT_TRUE((TestLdr<uint32_t>(this)));
    EXPECT_TRUE((TestLdr<uint64_t>(this)));
    EXPECT_TRUE((TestLdr<float>(this)));
    EXPECT_TRUE((TestLdr<double>(this)));

    // NOTE(igorban) : additional test for full memory model:
    //                 + mem(base + index<<scale + disp)
}

template <typename T>
bool TestStr(Encoder64Test *test)
{
    // Initialize
    test->PreWork();
    // First type-dependency
    auto param = test->GetParameter(TypeInfo(int64_t(0)), 0);
    // Data to be stored:
    auto storedValue = test->GetParameter(TypeInfo(T(0)), 1);
    if constexpr ((std::is_same<float, T>::value) || std::is_same<double, T>::value) {
        storedValue = test->GetParameter(TypeInfo(T(0)), 0);
    }

    auto mem = MemRef(param);
    test->GetEncoder()->EncodeStr(storedValue, mem);
    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp = RandomGen<T>();
        T retData = 0;
        T *ptr = &retData;

        // Test : param - Pointer to value
        //        return - value (loaded by ptr)
        // Value is resulting type, but call is ptr_type
        auto result = test->CallCodeStore<T>(reinterpret_cast<int64_t>(ptr), tmp);
        // Store must change ret_data value
        if (!(retData == tmp || (std::is_floating_point_v<T> && std::isnan(retData) && std::isnan(tmp)))) {
            std::cerr << std::hex << "Ldr test fail " << tmp << " ret_data = " << retData << "\n";
            return false;
        }
    }
    return true;
}

// Simple store test
TEST_F(Encoder64Test, StrTest)
{
    EXPECT_TRUE((TestStr<int32_t>(this)));
    EXPECT_TRUE((TestStr<int64_t>(this)));
    EXPECT_TRUE((TestStr<uint32_t>(this)));
    EXPECT_TRUE((TestStr<uint64_t>(this)));
    EXPECT_TRUE((TestStr<float>(this)));
    EXPECT_TRUE((TestStr<double>(this)));

    // NOTE(igorban) : additional test for full memory model:
    //                 + mem(base + index<<scale + disp)
}

// Store immediate test
// TEST_F(Encoder64Test, StiTest) {
//  EncodeSti(Imm src, MemRef mem)

// Store zero upper test
// TEST_F(Encoder64Test, StrzTest) {
//  EncodeStrz(Reg src, MemRef mem)

// Return test ???? What here may be tested
// TEST_F(Encoder64Test, ReturnTest) {
//  EncodeReturn()

bool Foo(uint32_t param1, uint32_t param2)
{
    // NOTE(igorban): use variables
    return (param1 == param2);
}

using FunctPtr = bool (*)(uint32_t param1, uint32_t param2);

FunctPtr g_fooPtr = &Foo;

// Call Test
TEST_F(Encoder64Test, CallTest)
{
    PreWork();

    // Call foo
    GetEncoder()->MakeCall(reinterpret_cast<void *>(g_fooPtr));

    // return value - moved to return value
    PostWork<float>();

    // If encode unsupported - now print error
    if (!GetEncoder()->GetResult()) {
        std::cerr << "Unsupported Call-instruction \n";
        return;
    }
    // Change this for enable print disasm
    Dump(false);
    // Equality test
    auto tmp1 = RandomGen<uint32_t>();
    uint32_t tmp2 = tmp1;
    // first template arg - parameter type, second - return type
    auto result = CallCodeCall<uint32_t, bool>(tmp1, tmp2);
    // Store must change ret_data value
    if (!result) {
        std::cerr << std::hex << "Call test fail tmp1=" << tmp1 << " tmp2=" << tmp2 << " result =" << result << "\n";
    }
    EXPECT_TRUE(result);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        auto tmp1 = RandomGen<uint32_t>();
        auto tmp2 = RandomGen<uint32_t>();

        // first template arg - parameter type, second - return type
        auto result = CallCodeCall<uint32_t, bool>(tmp1, tmp2);
        auto retData = (tmp1 == tmp2);

        // Store must change ret_data value
        if (result != retData) {
            std::cerr << std::hex << "Call test fail tmp1=" << tmp1 << " tmp2=" << tmp2 << " ret_data = " << retData
                      << " result =" << result << "\n";
        }
        EXPECT_EQ(result, retData);
    }
}

template <typename T>
bool TestAbs(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param = test->GetParameter(TypeInfo(T(0)));

    // Main test call
    test->GetEncoder()->EncodeAbs(param, param);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp = RandomGen<T>();
        // Main check - compare parameter and
        // return value
        if (!test->CallCode(tmp, std::abs(tmp))) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T>(nan, nan)) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, AbsTest)
{
    EXPECT_TRUE(TestAbs<int32_t>(this));
    EXPECT_TRUE(TestAbs<int64_t>(this));
    EXPECT_TRUE(TestAbs<float>(this));
    EXPECT_TRUE(TestAbs<double>(this));
}

template <typename T>
bool TestAdd(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeAdd(param1, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        auto tmp1 = RandomGen<T>();
        auto tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter

        T result {0};
        if constexpr (std::is_integral_v<T>) {
            result = T(std::make_unsigned_t<T>(tmp1) + tmp2);
        } else {
            result = tmp1 + tmp2;
        }

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, tmp2, result)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T>(nan, RandomGen<T>(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(RandomGen<T>(), nan, nan)) {
            return false;
        }
        if (!test->CallCode<T>(-std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity(), nan)) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, AddTest)
{
    EXPECT_TRUE(TestAdd<int32_t>(this));
    EXPECT_TRUE(TestAdd<int64_t>(this));
    EXPECT_TRUE(TestAdd<float>(this));
    EXPECT_TRUE(TestAdd<double>(this));
}

template <typename T>
bool TestAddImm(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    T param2 = RandomGen<T>();

    // Main test call
    test->GetEncoder()->EncodeAdd(param1, param1, Imm(param2));

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        // Deduced conflicting types for parameter

        T result {0};
        if constexpr (std::is_integral_v<T>) {
            result = T(std::make_unsigned_t<T>(tmp1) + param2);
        } else {
            result = tmp1 + param2;
        }

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, result)) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, AddImmTest)
{
    EXPECT_TRUE(TestAddImm<int32_t>(this));
    EXPECT_TRUE(TestAddImm<int64_t>(this));
    // TestAddImm<float>
    // TestAddImm<double>
}

template <typename T>
bool TestSub(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeSub(param1, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter

        T result {0};
        if constexpr (std::is_integral_v<T>) {
            result = T(std::make_unsigned_t<T>(tmp1) - tmp2);
        } else {
            result = tmp1 - tmp2;
        }

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, tmp2, result)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T>(nan, RandomGen<T>(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(RandomGen<T>(), nan, nan)) {
            return false;
        }
        if (!test->CallCode<T>(std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(-std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity(), nan)) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, SubTest)
{
    EXPECT_TRUE(TestSub<int32_t>(this));
    EXPECT_TRUE(TestSub<int64_t>(this));
    EXPECT_TRUE(TestSub<float>(this));
    EXPECT_TRUE(TestSub<double>(this));
}

template <typename T>
bool TestSubImm(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    T param2 = RandomGen<T>();

    // Main test call
    test->GetEncoder()->EncodeSub(param1, param1, Imm(param2));

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        // Deduced conflicting types for parameter

        T result {0};
        if constexpr (std::is_integral_v<T>) {
            result = T(std::make_unsigned_t<T>(tmp1) - param2);
        } else {
            result = tmp1 - param2;
        }

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, result)) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, SubImmTest)
{
    EXPECT_TRUE(TestSubImm<int32_t>(this));
    EXPECT_TRUE(TestSubImm<int64_t>(this));
    // TestSubImm<float>
    // TestSubImm<double>
}

template <typename T>
bool TestMul(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeMul(param1, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter

        T result {0};
        if constexpr (std::is_integral_v<T>) {
            result = T(std::make_unsigned_t<T>(tmp1) * tmp2);
        } else {
            result = tmp1 * tmp2;
        }

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, tmp2, result)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T>(nan, RandomGen<T>(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(RandomGen<T>(), nan, nan)) {
            return false;
        }
        if (!test->CallCode<T>(0.0, std::numeric_limits<T>::infinity(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(std::numeric_limits<T>::infinity(), 0.0, nan)) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, MulTest)
{
    EXPECT_TRUE(TestMul<int32_t>(this));
    EXPECT_TRUE(TestMul<int64_t>(this));
    EXPECT_TRUE(TestMul<float>(this));
    EXPECT_TRUE(TestMul<double>(this));
}

template <typename T>
bool TestMin(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeMin(param1, std::is_signed_v<T>, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter

        T result {0};
        if (std::is_floating_point_v<T> && (std::isnan(tmp1) || std::isnan(tmp2))) {
            result = std::numeric_limits<T>::quiet_NaN();
        } else {
            result = std::min(tmp1, tmp2);
        }

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, tmp2, result)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T>(nan, RandomGen<T>(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(RandomGen<T>(), nan, nan)) {
            return false;
        }
        // use static_cast to make sure correct float/double type is applied
        if (!test->CallCode<T>(static_cast<T>(-0.0), static_cast<T>(+0.0), static_cast<T>(-0.0))) {
            return false;
        }
        if (!test->CallCode<T>(static_cast<T>(+0.0), static_cast<T>(-0.0), static_cast<T>(-0.0))) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, MinTest)
{
    EXPECT_TRUE(TestMin<int32_t>(this));
    EXPECT_TRUE(TestMin<int64_t>(this));
    EXPECT_TRUE(TestMin<uint32_t>(this));
    EXPECT_TRUE(TestMin<uint64_t>(this));
    EXPECT_TRUE(TestMin<float>(this));
    EXPECT_TRUE(TestMin<double>(this));
}

template <typename T>
bool TestMax(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeMax(param1, std::is_signed_v<T>, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter
        T result {0};
        if (std::is_floating_point_v<T> && (std::isnan(tmp1) || std::isnan(tmp2))) {
            result = std::numeric_limits<T>::quiet_NaN();
        } else {
            result = std::max(tmp1, tmp2);
        }

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, tmp2, result)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T, T>(nan, RandomGen<T>(), nan)) {
            return false;
        }
        if (!test->CallCode<T, T>(RandomGen<T>(), nan, nan)) {
            return false;
        }
        // use static_cast to make sure correct float/double type is applied
        if (!test->CallCode<T>(static_cast<T>(-0.0), static_cast<T>(+0.0), static_cast<T>(+0.0))) {
            return false;
        }
        if (!test->CallCode<T>(static_cast<T>(+0.0), static_cast<T>(-0.0), static_cast<T>(+0.0))) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, MaxTest)
{
    EXPECT_TRUE(TestMax<int32_t>(this));
    EXPECT_TRUE(TestMax<int64_t>(this));
    EXPECT_TRUE(TestMax<uint32_t>(this));
    EXPECT_TRUE(TestMax<uint64_t>(this));
    EXPECT_TRUE(TestMax<float>(this));
    EXPECT_TRUE(TestMax<double>(this));
}

template <typename T>
bool TestShl(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeShl(param1, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 {0};
        if constexpr (std::is_same_v<T, int64_t>) {
            tmp2 = RandomGen<uint8_t>() % DOUBLE_WORD_SIZE;
        } else {
            tmp2 = RandomGen<uint8_t>() % WORD_SIZE;
        }
        // Deduced conflicting types for parameter

        // Main check - compare parameter and
        // return value
        bool result {false};

        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t>) {
            result =
                test->CallCode<T>(tmp1, tmp2, std::make_unsigned_t<T>(tmp1) << (tmp2 & (CHAR_BIT * sizeof(T) - 1)));
        } else {
            result =
                test->CallCode<T>(tmp1, tmp2, std::make_unsigned_t<T>(tmp1) << tmp2);  // NOLINT(hicpp-signed-bitwise)
        }

        if (!result) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, ShlTest)
{
    EXPECT_TRUE(TestShl<int32_t>(this));
    EXPECT_TRUE(TestShl<int64_t>(this));
}

template <typename T>
bool TestShr(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeShr(param1, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 {0};
        if constexpr (sizeof(T) == sizeof(int64_t)) {
            tmp2 = RandomGen<uint8_t>() % DOUBLE_WORD_SIZE;
        } else {
            tmp2 = RandomGen<uint8_t>() % WORD_SIZE;
        }
        // Deduced conflicting types for parameter

        // Main check - compare parameter and
        // return value
        bool result {false};

        if constexpr (sizeof(T) == sizeof(int64_t)) {
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            result = test->CallCode<T>(tmp1, tmp2, (static_cast<uint64_t>(tmp1)) >> tmp2);
        } else {
            result =
                // NOLINTNEXTLINE(hicpp-signed-bitwise)
                test->CallCode<T>(tmp1, tmp2, (static_cast<uint32_t>(tmp1)) >> (tmp2 & (CHAR_BIT * sizeof(T) - 1)));
        }

        if (!result) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, ShrTest)
{
    EXPECT_TRUE(TestShr<int32_t>(this));
    EXPECT_TRUE(TestShr<int64_t>(this));
    EXPECT_TRUE(TestShr<uint32_t>(this));
    EXPECT_TRUE(TestShr<uint64_t>(this));
}

template <typename T>
bool TestAShr(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeAShr(param1, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 {0};
        if constexpr (std::is_same_v<T, int64_t>) {
            tmp2 = RandomGen<uint8_t>() % DOUBLE_WORD_SIZE;
        } else {
            tmp2 = RandomGen<uint8_t>() % WORD_SIZE;
        }
        // Deduced conflicting types for parameter

        // Main check - compare parameter and
        // return value
        bool result {false};

        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t>) {
            result = test->CallCode<T>(tmp1, tmp2, tmp1 >> (tmp2 & (CHAR_BIT * sizeof(T) - 1)));
        } else {
            result = test->CallCode<T>(tmp1, tmp2, tmp1 >> tmp2);  // NOLINT(hicpp-signed-bitwise)
        }

        if (!result) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, AShrTest)
{
    EXPECT_TRUE(TestAShr<int32_t>(this));
    EXPECT_TRUE(TestAShr<int64_t>(this));
}

template <typename T>
bool TestAnd(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeAnd(param1, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter
        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, tmp2, tmp1 & tmp2)) {  // NOLINT(hicpp-signed-bitwise)
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, AndTest)
{
    EXPECT_TRUE(TestAnd<int32_t>(this));
    EXPECT_TRUE(TestAnd<int64_t>(this));
}

template <typename T>
bool TestOr(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeOr(param1, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter
        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, tmp2, tmp1 | tmp2)) {  // NOLINT(hicpp-signed-bitwise)
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, OrTest)
{
    EXPECT_TRUE(TestOr<int32_t>(this));
    EXPECT_TRUE(TestOr<int64_t>(this));
}

template <typename T>
bool TestXor(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeXor(param1, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter
        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, tmp2, tmp1 ^ tmp2)) {  // NOLINT(hicpp-signed-bitwise)
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, XorTest)
{
    EXPECT_TRUE(TestXor<int32_t>(this));
    EXPECT_TRUE(TestXor<int64_t>(this));
}

template <typename T>
bool TestShlImm(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    int64_t param2 {0};
    if constexpr (std::is_same_v<T, int64_t>) {
        param2 = RandomGen<uint8_t>() % DOUBLE_WORD_SIZE;
    } else {
        param2 = RandomGen<uint8_t>() % WORD_SIZE;
    }

    // Main test call
    test->GetEncoder()->EncodeShl(param1, param1, Imm(param2));

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();

        // Deduced conflicting types for parameter

        // Main check - compare parameter and
        // return value
        bool result {false};

        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t>) {
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            result = test->CallCode<T>(tmp1, std::make_unsigned_t<T>(tmp1) << (param2 & (CHAR_BIT * sizeof(T) - 1)));
        } else {
            result = test->CallCode<T>(tmp1, std::make_unsigned_t<T>(tmp1) << param2);  // NOLINT(hicpp-signed-bitwise)
        }

        if (!result) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, ShlImmTest)
{
    EXPECT_TRUE(TestShlImm<int32_t>(this));
    EXPECT_TRUE(TestShlImm<int64_t>(this));
}

template <typename T>
bool TestShrImm(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    int64_t param2 {0};
    if constexpr (sizeof(T) == sizeof(int64_t)) {
        param2 = RandomGen<uint8_t>() % DOUBLE_WORD_SIZE;
    } else {
        param2 = RandomGen<uint8_t>() % WORD_SIZE;
    }

    // Main test call
    test->GetEncoder()->EncodeShr(param1, param1, Imm(param2));

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();

        // Deduced conflicting types for parameter

        // Main check - compare parameter and
        // return value
        bool result {false};

        if constexpr (sizeof(T) == sizeof(int64_t)) {
            result = test->CallCode<T>(tmp1, (static_cast<uint64_t>(tmp1)) >> param2);  // NOLINT(hicpp-signed-bitwise)
        } else {
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            result = test->CallCode<T>(tmp1, (static_cast<uint32_t>(tmp1)) >> (param2 & (CHAR_BIT * sizeof(T) - 1)));
        }

        if (!result) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, ShrImmTest)
{
    EXPECT_TRUE(TestShrImm<int32_t>(this));
    EXPECT_TRUE(TestShrImm<int64_t>(this));
    EXPECT_TRUE(TestShrImm<uint32_t>(this));
    EXPECT_TRUE(TestShrImm<uint64_t>(this));
}

template <typename T>
bool TestCmp(Encoder64Test *test)
{
    static_assert(std::is_integral_v<T>);
    // Initialize
    test->PreWork();

    // First type-dependency
    auto output = test->GetParameter(TypeInfo(int32_t(0)), 0);
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeCmp(output, param1, param2, std::is_signed_v<T> ? Condition::LT : Condition::LO);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter

        auto compare = [](T a, T b) -> int32_t { return a < b ? -1 : a > b ? 1 : 0; };
        int32_t result {compare(tmp1, tmp2)};

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T, int32_t>(tmp1, tmp2, result)) {
            return false;
        }
    }

    return true;
}

template <typename T>
bool TestFcmp(Encoder64Test *test, bool isFcmpg)
{
    static_assert(std::is_floating_point_v<T>);
    // Initialize
    test->PreWork();

    // First type-dependency
    auto output = test->GetParameter(TypeInfo(int32_t(0)), 0);
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeCmp(output, param1, param2, isFcmpg ? Condition::MI : Condition::LT);

    // Finalize
    test->PostWork<int>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter

        auto compare = [](T a, T b) -> int32_t { return a < b ? -1 : a > b ? 1 : 0; };

        int32_t result {0};
        if (std::isnan(tmp1) || std::isnan(tmp2)) {
            result = isFcmpg ? 1 : -1;
        } else {
            result = compare(tmp1, tmp2);
        }

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T, int32_t>(tmp1, tmp2, result)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T, int32_t>(nan, 5.0, isFcmpg ? 1 : -1)) {
            return false;
        }
        if (!test->CallCode<T, int32_t>(5.0, nan, isFcmpg ? 1 : -1)) {
            return false;
        }
        if (!test->CallCode<T, int32_t>(nan, nan, isFcmpg ? 1 : -1)) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, CmpTest)
{
    EXPECT_TRUE(TestCmp<int32_t>(this));
    EXPECT_TRUE(TestCmp<int64_t>(this));
    EXPECT_TRUE(TestCmp<uint32_t>(this));
    EXPECT_TRUE(TestCmp<uint64_t>(this));
    EXPECT_TRUE(TestFcmp<float>(this, false));
    EXPECT_TRUE(TestFcmp<double>(this, false));
    EXPECT_TRUE(TestFcmp<float>(this, true));
    EXPECT_TRUE(TestFcmp<double>(this, true));
}

template <typename T>
bool TestCmp64(Encoder64Test *test)
{
    static_assert(sizeof(T) == sizeof(int64_t));
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeCmp(param1, param1, param2, std::is_signed_v<T> ? Condition::LT : Condition::LO);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    uint64_t hi = static_cast<uint64_t>(0xABCDEFU) << (BITS_PER_BYTE * sizeof(uint32_t));

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        uint32_t lo = RandomGen<T>();

        // Second type-dependency
        T tmp1 = hi | (lo + 1U);
        T tmp2 = hi | lo;
        // Deduced conflicting types for parameter

        auto compare = [](T a, T b) -> int32_t { return a < b ? -1 : a > b ? 1 : 0; };

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T, int32_t>(tmp1, tmp2, compare(tmp1, tmp2))) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, Cmp64Test)
{
    EXPECT_TRUE(TestCmp64<int64_t>(this));
    EXPECT_TRUE(TestCmp64<uint64_t>(this));
}

template <typename T>
bool TestCompare(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);
    auto retVal = std::is_floating_point_v<T> ? Target::Current().GetReturnReg() : Target::Current().GetParamReg(0);

    // Main test call
    test->GetEncoder()->EncodeCompare(retVal, param1, param2, std::is_signed_v<T> ? Condition::GE : Condition::HS);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter

        auto compare = [](T a, T b) -> int32_t { return a >= b; };

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T, int32_t>(tmp1, tmp2, compare(tmp1, tmp2))) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, CompareTest)
{
    EXPECT_TRUE(TestCompare<int32_t>(this));
    EXPECT_TRUE(TestCompare<int64_t>(this));
    EXPECT_TRUE(TestCompare<uint32_t>(this));
    EXPECT_TRUE(TestCompare<uint64_t>(this));
    EXPECT_TRUE(TestCompare<float>(this));
    EXPECT_TRUE(TestCompare<double>(this));
}

template <typename T>
bool TestCompare64(Encoder64Test *test)
{
    static_assert(sizeof(T) == sizeof(int64_t));
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeCompare(param1, param1, param2, std::is_signed_v<T> ? Condition::LT : Condition::LO);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    uint64_t hi = static_cast<uint64_t>(0xABCDEFU) << (BITS_PER_BYTE * sizeof(uint32_t));

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        uint32_t lo = RandomGen<T>();

        // Second type-dependency
        T tmp1 = hi | (lo + 1U);
        T tmp2 = hi | lo;
        // Deduced conflicting types for parameter

        auto compare = [](T a, T b) -> int32_t { return a < b; };

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T, int32_t>(tmp1, tmp2, compare(tmp1, tmp2))) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, Compare64Test)
{
    EXPECT_TRUE(TestCompare64<int64_t>(this));
    EXPECT_TRUE(TestCompare64<uint64_t>(this));
}

template <typename Src, typename Dst>
bool TestCast(Encoder64Test *test)
{
    // Initialize
    test->PreWork();
    // First type-dependency
    auto input = test->GetParameter(TypeInfo(Src(0)), 0);
    auto output = test->GetParameter(TypeInfo(Dst(0)), 0);
    // Main test call
    test->GetEncoder()->EncodeCast(output, std::is_signed_v<Dst>, input, std::is_signed_v<Src>);

    // Finalize
    test->PostWork<Dst>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<Src>() << ", " << TypeName<Dst>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Using max size type: type result "Dst" or 32bit to check result,
        // because in our ISA min type is 32bit.
        // Only integers less thаn 32bit.
        using DstExt = typename std::conditional<(sizeof(Dst) * BYTE_SIZE) >= WORD_SIZE, Dst, uint32_t>::type;

        // Second type-dependency
        auto src = RandomGen<Src>();
        DstExt dst {};

        if (std::is_floating_point<Src>() && !std::is_floating_point<Dst>()) {
            auto minInt = std::numeric_limits<Dst>::min();
            auto maxInt = std::numeric_limits<Dst>::max();
            auto floatMinInt = static_cast<Src>(minInt);
            auto floatMaxInt = static_cast<Src>(maxInt);

            if (src > floatMinInt) {
                dst = src < floatMaxInt ? static_cast<Dst>(src) : maxInt;
            } else if (std::isnan(src)) {
                dst = 0;
            } else {
                dst = minInt;
            }
        } else {
            dst = static_cast<DstExt>(static_cast<Dst>(src));
        }

        // Deduced conflicting types for parameter

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<Src, DstExt>(src, dst)) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<Src> && std::is_integral_v<Dst>) {
        Src nan = std::numeric_limits<Src>::quiet_NaN();
        using DstExt = typename std::conditional<(sizeof(Dst) * BYTE_SIZE) >= WORD_SIZE, Dst, uint32_t>::type;
        if (!test->CallCode<Src, DstExt>(nan, Dst(0))) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, CastTest)
{
    // Test int8
    EXPECT_TRUE((TestCast<int8_t, int8_t>(this)));
    EXPECT_TRUE((TestCast<int8_t, int16_t>(this)));
    EXPECT_TRUE((TestCast<int8_t, int32_t>(this)));
    EXPECT_TRUE((TestCast<int8_t, int64_t>(this)));

    EXPECT_TRUE((TestCast<int8_t, uint8_t>(this)));
    EXPECT_TRUE((TestCast<int8_t, uint16_t>(this)));
    EXPECT_TRUE((TestCast<int8_t, uint32_t>(this)));
    EXPECT_TRUE((TestCast<int8_t, uint64_t>(this)));
    EXPECT_TRUE((TestCast<int8_t, float>(this)));
    EXPECT_TRUE((TestCast<int8_t, double>(this)));

    EXPECT_TRUE((TestCast<uint8_t, int8_t>(this)));
    EXPECT_TRUE((TestCast<uint8_t, int16_t>(this)));
    EXPECT_TRUE((TestCast<uint8_t, int32_t>(this)));
    EXPECT_TRUE((TestCast<uint8_t, int64_t>(this)));

    EXPECT_TRUE((TestCast<uint8_t, uint8_t>(this)));
    EXPECT_TRUE((TestCast<uint8_t, uint16_t>(this)));
    EXPECT_TRUE((TestCast<uint8_t, uint32_t>(this)));
    EXPECT_TRUE((TestCast<uint8_t, uint64_t>(this)));
    EXPECT_TRUE((TestCast<uint8_t, float>(this)));
    EXPECT_TRUE((TestCast<uint8_t, double>(this)));

    // Test int16
    EXPECT_TRUE((TestCast<int16_t, int8_t>(this)));
    EXPECT_TRUE((TestCast<int16_t, int16_t>(this)));
    EXPECT_TRUE((TestCast<int16_t, int32_t>(this)));
    EXPECT_TRUE((TestCast<int16_t, int64_t>(this)));

    EXPECT_TRUE((TestCast<int16_t, uint8_t>(this)));
    EXPECT_TRUE((TestCast<int16_t, uint16_t>(this)));
    EXPECT_TRUE((TestCast<int16_t, uint32_t>(this)));
    EXPECT_TRUE((TestCast<int16_t, uint64_t>(this)));
    EXPECT_TRUE((TestCast<int16_t, float>(this)));
    EXPECT_TRUE((TestCast<int16_t, double>(this)));

    EXPECT_TRUE((TestCast<uint16_t, int8_t>(this)));
    EXPECT_TRUE((TestCast<uint16_t, int16_t>(this)));
    EXPECT_TRUE((TestCast<uint16_t, int32_t>(this)));
    EXPECT_TRUE((TestCast<uint16_t, int64_t>(this)));

    EXPECT_TRUE((TestCast<uint16_t, uint8_t>(this)));
    EXPECT_TRUE((TestCast<uint16_t, uint16_t>(this)));
    EXPECT_TRUE((TestCast<uint16_t, uint32_t>(this)));
    EXPECT_TRUE((TestCast<uint16_t, uint64_t>(this)));
    EXPECT_TRUE((TestCast<uint16_t, float>(this)));
    EXPECT_TRUE((TestCast<uint16_t, double>(this)));

    // Test int32
    EXPECT_TRUE((TestCast<int32_t, int8_t>(this)));
    EXPECT_TRUE((TestCast<int32_t, int16_t>(this)));
    EXPECT_TRUE((TestCast<int32_t, int32_t>(this)));
    EXPECT_TRUE((TestCast<int32_t, int64_t>(this)));

    EXPECT_TRUE((TestCast<int32_t, uint8_t>(this)));
    EXPECT_TRUE((TestCast<int32_t, uint16_t>(this)));
    EXPECT_TRUE((TestCast<int32_t, uint32_t>(this)));
    EXPECT_TRUE((TestCast<int32_t, uint64_t>(this)));
    EXPECT_TRUE((TestCast<int32_t, float>(this)));
    EXPECT_TRUE((TestCast<int32_t, double>(this)));

    EXPECT_TRUE((TestCast<uint32_t, int8_t>(this)));
    EXPECT_TRUE((TestCast<uint32_t, int16_t>(this)));
    EXPECT_TRUE((TestCast<uint32_t, int32_t>(this)));
    EXPECT_TRUE((TestCast<uint32_t, int64_t>(this)));

    EXPECT_TRUE((TestCast<uint32_t, uint8_t>(this)));
    EXPECT_TRUE((TestCast<uint32_t, uint16_t>(this)));
    EXPECT_TRUE((TestCast<uint32_t, uint32_t>(this)));
    EXPECT_TRUE((TestCast<uint32_t, uint64_t>(this)));
    EXPECT_TRUE((TestCast<uint32_t, float>(this)));
    EXPECT_TRUE((TestCast<uint32_t, double>(this)));

    // Test int64
    EXPECT_TRUE((TestCast<int64_t, int8_t>(this)));
    EXPECT_TRUE((TestCast<int64_t, int16_t>(this)));
    EXPECT_TRUE((TestCast<int64_t, int32_t>(this)));
    EXPECT_TRUE((TestCast<int64_t, int64_t>(this)));

    EXPECT_TRUE((TestCast<int64_t, uint8_t>(this)));
    EXPECT_TRUE((TestCast<int64_t, uint16_t>(this)));
    EXPECT_TRUE((TestCast<int64_t, uint32_t>(this)));
    EXPECT_TRUE((TestCast<int64_t, uint64_t>(this)));
    EXPECT_TRUE((TestCast<int64_t, float>(this)));
    EXPECT_TRUE((TestCast<int64_t, double>(this)));

    EXPECT_TRUE((TestCast<uint64_t, int8_t>(this)));
    EXPECT_TRUE((TestCast<uint64_t, int16_t>(this)));
    EXPECT_TRUE((TestCast<uint64_t, int32_t>(this)));
    EXPECT_TRUE((TestCast<uint64_t, int64_t>(this)));

    EXPECT_TRUE((TestCast<uint64_t, uint8_t>(this)));
    EXPECT_TRUE((TestCast<uint64_t, uint16_t>(this)));
    EXPECT_TRUE((TestCast<uint64_t, uint32_t>(this)));
    EXPECT_TRUE((TestCast<uint64_t, uint64_t>(this)));
    EXPECT_TRUE((TestCast<uint64_t, float>(this)));
    EXPECT_TRUE((TestCast<uint64_t, double>(this)));

    // Test float/double
    EXPECT_TRUE((TestCast<float, float>(this)));
    EXPECT_TRUE((TestCast<double, double>(this)));
    EXPECT_TRUE((TestCast<float, double>(this)));
    EXPECT_TRUE((TestCast<double, float>(this)));

    // We DON'T support cast from float32/64 to int8/16.

    EXPECT_TRUE((TestCast<float, int32_t>(this)));
    EXPECT_TRUE((TestCast<float, int64_t>(this)));
    EXPECT_TRUE((TestCast<float, uint32_t>(this)));
    EXPECT_TRUE((TestCast<float, uint64_t>(this)));

    EXPECT_TRUE((TestCast<double, int32_t>(this)));
    EXPECT_TRUE((TestCast<double, int64_t>(this)));
    EXPECT_TRUE((TestCast<double, uint32_t>(this)));
    EXPECT_TRUE((TestCast<double, uint64_t>(this)));
}

template <typename T>
bool TestDiv(Encoder64Test *test)
{
    bool isSigned = std::is_signed<T>::value;

    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeDiv(param1, isSigned, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        if (tmp2 == 0) {
            tmp2 += 1;
        }
        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T>(tmp1, tmp2, (tmp1 / tmp2))) {
            return false;
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T>(nan, RandomGen<T>(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(RandomGen<T>(), nan, nan)) {
            return false;
        }
        if (!test->CallCode<T>(0.0, 0.0, nan)) {
            return false;
        }
        if (!test->CallCode<T>(std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(-std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity(), nan)) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, DivTest)
{
    EXPECT_TRUE(TestDiv<int32_t>(this));
    EXPECT_TRUE(TestDiv<int64_t>(this));
    EXPECT_TRUE(TestDiv<uint32_t>(this));
    EXPECT_TRUE(TestDiv<uint64_t>(this));
    EXPECT_TRUE(TestDiv<float>(this));
    EXPECT_TRUE(TestDiv<double>(this));
}

template <typename T>
bool TestMod(Encoder64Test *test)
{
    bool isSigned = std::is_signed<T>::value;

    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeMod(param1, isSigned, param1, param2);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        if (tmp2 == 0) {
            tmp2 += 1;
        }
        // Main check - compare parameter and
        // return value
        if constexpr (std::is_same<float, T>::value) {
            if (!test->CallCode<T>(tmp1, tmp2, fmodf(tmp1, tmp2))) {
                return false;
            }
        } else if constexpr (std::is_same<double, T>::value) {
            if (!test->CallCode<T>(tmp1, tmp2, fmod(tmp1, tmp2))) {
                return false;
            }
        } else {
            if (!test->CallCode<T>(tmp1, tmp2, static_cast<T>(tmp1 % tmp2))) {
                return false;
            }
        }
    }

    if constexpr (std::is_floating_point_v<T>) {
        T nan = std::numeric_limits<T>::quiet_NaN();
        if (!test->CallCode<T>(nan, RandomGen<T>(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(RandomGen<T>(), nan, nan)) {
            return false;
        }
        if (!test->CallCode<T>(0.0, 0.0, nan)) {
            return false;
        }
        if (!test->CallCode<T>(std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity(), nan)) {
            return false;
        }
        if (!test->CallCode<T>(-std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity(), nan)) {
            return false;
        }
    }

    return true;
}

TEST_F(Encoder64Test, ModTest)
{
    EXPECT_TRUE(TestMod<int32_t>(this));
    EXPECT_TRUE(TestMod<int64_t>(this));
    EXPECT_TRUE(TestMod<uint32_t>(this));
    EXPECT_TRUE(TestMod<uint64_t>(this));
    EXPECT_TRUE(TestMod<float>(this));
    EXPECT_TRUE(TestMod<double>(this));
}

// MemCopy Test
// TEST_F(Encoder64Test, MemCopyTest) {
//  EncodeMemCopy(MemRef mem_from, MemRef mem_to, size_t size)

// MemCopyz Test
// TEST_F(Encoder64Test, MemCopyzTest) {
//  EncodeMemCopyz(MemRef mem_from, MemRef mem_to, size_t size)

// int32_t uint64_t int32_t  int64_t         int32_t int32_t
//   r0    r2+r3   stack0  stack2(align)   stack4
using FunctionPtr = uint64_t (*)(uint32_t, uint64_t, int32_t, int64_t, int32_t, int32_t);

template <int ID, typename T>
bool TestParamMainLoop(FunctionPtr func)
{
    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        auto param0 = RandomGen<uint32_t>();
        auto param1 = RandomGen<uint64_t>();
        auto param2 = RandomGen<int32_t>();
        auto param3 = RandomGen<int64_t>();
        auto param4 = RandomGen<int32_t>();
        auto param5 = RandomGen<int32_t>();

        // Main check - compare parameter and
        // return value
        const T currResult = func(param0, param1, param2, param3, param4, param5);
        T result;
        if constexpr (ID == 0) {
            result = param0;
        }
        if constexpr (ID == 1) {
            result = param1;
        }
        if constexpr (ID == 2) {
            result = param2;
        }
        if constexpr (ID == 3) {
            result = param3;
        }
        if constexpr (ID == 4) {
            result = param4;
        }
        if constexpr (ID == 5) {
            result = param5;
        }

        if (currResult != result) {
            return false;
        };
    }
    return true;
}

template <int ID, typename T>
bool TestParam(Encoder64Test *test)
{
    bool isSigned = std::is_signed<T>::value;

    constexpr std::array<TypeInfo, 6> PARAMS {INT32_TYPE, INT64_TYPE, INT32_TYPE, INT64_TYPE, INT32_TYPE, INT32_TYPE};
    TypeInfo currParam = PARAMS[ID];

    auto paramInfo = test->GetCallconv()->GetParameterInfo(0);
    auto par = paramInfo->GetNativeParam(PARAMS[0]);
    // First ret value geted
    for (int i = 1; i <= ID; ++i) {
        par = paramInfo->GetNativeParam(PARAMS[i]);
    }

    test->PreWork();

    auto retReg = test->GetParameter(currParam, 0);

    // Main test call
    if (std::holds_alternative<Reg>(par)) {
        test->GetEncoder()->EncodeMov(retReg, std::get<Reg>(par));
    } else {
        auto mem = MemRef(Target::Current().GetStackReg(), std::get<uint8_t>(par) * sizeof(uint32_t));
        test->GetEncoder()->EncodeLdr(retReg, isSigned, mem);
    }

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported parameter with " << ID << "\n";
        return false;
    }

    // Finalize
    test->PostWork<T>();

    // Change this for enable print disasm
    test->Dump(false);

    auto size = test->GetCallconv()->GetCodeSize() - test->GetCursor();
    void *offset = (static_cast<uint8_t *>(test->GetCallconv()->GetCodeEntry()));
    void *ptr = test->GetCodeAllocator()->AllocateCode(size, offset);
    auto func = reinterpret_cast<FunctionPtr>(ptr);

    return TestParamMainLoop<ID, T>(func);
}

TEST_F(Encoder64Test, ReadParams)
{
    EXPECT_TRUE((TestParam<0, int32_t>(this)));
    EXPECT_TRUE((TestParam<1, uint64_t>(this)));
    EXPECT_TRUE((TestParam<2, int32_t>(this)));
    EXPECT_TRUE((TestParam<3, int64_t>(this)));
    EXPECT_TRUE((TestParam<4, int32_t>(this)));
    EXPECT_TRUE((TestParam<5, int32_t>(this)));
}

template <typename T, Condition CC>
bool TestSelect(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param0 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param1 = test->GetParameter(TypeInfo(T(0)), 1);
    auto param2 = test->GetParameter(TypeInfo(uint32_t(0)), 2);
    auto param3 = test->GetParameter(TypeInfo(uint32_t(0)), 3);

    // Main test call
    test->GetEncoder()->EncodeMov(param2, Imm(1));
    test->GetEncoder()->EncodeMov(param3, Imm(0));
    test->GetEncoder()->EncodeSelect(Reg(param0.GetId(), TypeInfo(uint32_t(0))), param2, param3, param0, param1, CC);

    // Finalize
    test->PostWork<bool>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp0 = RandomGen<T>();
        T tmp1 = RandomGen<T>();

        bool res {false};
        switch (CC) {
            case Condition::LT:
            case Condition::LO:
                res = tmp0 < tmp1;
                break;
            case Condition::EQ:
                res = tmp0 == tmp1;
                break;
            case Condition::NE:
                res = tmp0 != tmp1;
                break;
            case Condition::GT:
            case Condition::HI:
                res = tmp0 > tmp1;
                break;
            default:
                UNREACHABLE();
        }

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T, bool>(tmp0, tmp1, res)) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, SelectTest)
{
    EXPECT_TRUE((TestSelect<uint32_t, Condition::LO>(this)));
    EXPECT_TRUE((TestSelect<uint32_t, Condition::EQ>(this)));
    EXPECT_TRUE((TestSelect<uint32_t, Condition::NE>(this)));
    EXPECT_TRUE((TestSelect<uint32_t, Condition::HI>(this)));

    EXPECT_TRUE((TestSelect<uint64_t, Condition::LO>(this)));
    EXPECT_TRUE((TestSelect<uint64_t, Condition::EQ>(this)));
    EXPECT_TRUE((TestSelect<uint64_t, Condition::NE>(this)));
    EXPECT_TRUE((TestSelect<uint64_t, Condition::HI>(this)));

    EXPECT_TRUE((TestSelect<int32_t, Condition::LT>(this)));
    EXPECT_TRUE((TestSelect<int32_t, Condition::EQ>(this)));
    EXPECT_TRUE((TestSelect<int32_t, Condition::NE>(this)));
    EXPECT_TRUE((TestSelect<int32_t, Condition::GT>(this)));

    EXPECT_TRUE((TestSelect<int64_t, Condition::LT>(this)));
    EXPECT_TRUE((TestSelect<int64_t, Condition::EQ>(this)));
    EXPECT_TRUE((TestSelect<int64_t, Condition::NE>(this)));
    EXPECT_TRUE((TestSelect<int64_t, Condition::GT>(this)));
}

template <typename T, Condition CC, bool IS_IMM>
bool TestSelectTest(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    auto param0 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param1 = test->GetParameter(TypeInfo(T(0)), 1);
    auto param2 = test->GetParameter(TypeInfo(uint32_t(0)), 2);
    auto param3 = test->GetParameter(TypeInfo(uint32_t(0)), 3);
    [[maybe_unused]] T immValue = RandomGen<T>();

    // Main test call
    test->GetEncoder()->EncodeMov(param2, Imm(1));
    test->GetEncoder()->EncodeMov(param3, Imm(0));

    if constexpr (IS_IMM) {
        test->GetEncoder()->EncodeSelectTest(Reg(param0.GetId(), TypeInfo(uint32_t(0))), param2, param3, param0,
                                             Imm(immValue), CC);
    } else {
        test->GetEncoder()->EncodeSelectTest(Reg(param0.GetId(), TypeInfo(uint32_t(0))), param2, param3, param0, param1,
                                             CC);
    }

    // Finalize
    test->PostWork<bool>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        T tmp0 = RandomGen<T>();
        T tmp1;

        if constexpr (IS_IMM) {
            tmp1 = immValue;
        } else {
            tmp1 = RandomGen<T>();
        }

        T andRes = tmp0 & tmp1;                                          // NOLINT(hicpp-signed-bitwise)
        bool res = CC == Condition::TST_EQ ? andRes == 0 : andRes != 0;  // NOLINT(hicpp-signed-bitwise)

        // Main check - compare parameter and return value
        if (!test->CallCode<T, bool>(tmp0, tmp1, res)) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, SelectTestTest)
{
    EXPECT_TRUE((TestSelectTest<uint32_t, Condition::TST_EQ, false>(this)));
    EXPECT_TRUE((TestSelectTest<uint32_t, Condition::TST_NE, false>(this)));
    EXPECT_TRUE((TestSelectTest<uint32_t, Condition::TST_EQ, true>(this)));
    EXPECT_TRUE((TestSelectTest<uint32_t, Condition::TST_NE, true>(this)));

    EXPECT_TRUE((TestSelectTest<int32_t, Condition::TST_EQ, false>(this)));
    EXPECT_TRUE((TestSelectTest<int32_t, Condition::TST_NE, false>(this)));
    EXPECT_TRUE((TestSelectTest<int32_t, Condition::TST_EQ, true>(this)));
    EXPECT_TRUE((TestSelectTest<int32_t, Condition::TST_NE, true>(this)));

    EXPECT_TRUE((TestSelectTest<uint64_t, Condition::TST_EQ, false>(this)));
    EXPECT_TRUE((TestSelectTest<uint64_t, Condition::TST_NE, false>(this)));
    EXPECT_TRUE((TestSelectTest<uint64_t, Condition::TST_EQ, true>(this)));
    EXPECT_TRUE((TestSelectTest<uint64_t, Condition::TST_NE, true>(this)));

    EXPECT_TRUE((TestSelectTest<int64_t, Condition::TST_EQ, false>(this)));
    EXPECT_TRUE((TestSelectTest<int64_t, Condition::TST_NE, false>(this)));
    EXPECT_TRUE((TestSelectTest<int64_t, Condition::TST_EQ, true>(this)));
    EXPECT_TRUE((TestSelectTest<int64_t, Condition::TST_NE, true>(this)));
}

template <typename T, Condition CC>
bool TestCompareTest(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param1 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param2 = test->GetParameter(TypeInfo(T(0)), 1);

    // Main test call
    test->GetEncoder()->EncodeCompareTest(param1, param1, param2, CC);

    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2 = RandomGen<T>();
        // Deduced conflicting types for parameter

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        auto compare = [](T a, T b) -> int32_t { return CC == Condition::TST_EQ ? (a & b) == 0 : (a & b) != 0; };

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T, int32_t>(tmp1, tmp2, compare(tmp1, tmp2))) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, CompareTestTest)
{
    EXPECT_TRUE((TestCompareTest<int32_t, Condition::TST_EQ>(this)));
    EXPECT_TRUE((TestCompareTest<int32_t, Condition::TST_NE>(this)));
    EXPECT_TRUE((TestCompareTest<int64_t, Condition::TST_EQ>(this)));
    EXPECT_TRUE((TestCompareTest<int64_t, Condition::TST_NE>(this)));
    EXPECT_TRUE((TestCompareTest<uint32_t, Condition::TST_EQ>(this)));
    EXPECT_TRUE((TestCompareTest<uint32_t, Condition::TST_NE>(this)));
    EXPECT_TRUE((TestCompareTest<uint64_t, Condition::TST_EQ>(this)));
    EXPECT_TRUE((TestCompareTest<uint64_t, Condition::TST_NE>(this)));
}

template <typename T, Condition CC, bool IS_IMM>
bool TestJumpTest(Encoder64Test *test)
{
    // Initialize
    test->PreWork();

    // First type-dependency
    auto param0 = test->GetParameter(TypeInfo(T(0)), 0);
    auto param1 = test->GetParameter(TypeInfo(T(0)), 1);
    auto retVal = Target::Current().GetParamReg(0);

    auto trueBranch = test->GetEncoder()->CreateLabel();
    auto end = test->GetEncoder()->CreateLabel();
    [[maybe_unused]] T immValue = RandomGen<T>();

    // Main test call
    if constexpr (IS_IMM) {
        test->GetEncoder()->EncodeJumpTest(trueBranch, param0, Imm(immValue), CC);
    } else {
        test->GetEncoder()->EncodeJumpTest(trueBranch, param0, param1, CC);
    }
    test->GetEncoder()->EncodeMov(retVal, Imm(0));
    test->GetEncoder()->EncodeJump(end);

    test->GetEncoder()->BindLabel(trueBranch);
    test->GetEncoder()->EncodeMov(retVal, Imm(1));

    test->GetEncoder()->BindLabel(end);
    // Finalize
    test->PostWork<T>();

    // If encode unsupported - now print error
    if (!test->GetEncoder()->GetResult()) {
        std::cerr << "Unsupported for " << TypeName<T>() << "\n";
        return false;
    }
    // Change this for enable print disasm
    test->Dump(false);

    // Main test loop:
    for (uint64_t i = 0; i < ITERATION; ++i) {
        // Second type-dependency
        T tmp1 = RandomGen<T>();
        T tmp2;
        if constexpr (IS_IMM) {
            tmp2 = immValue;
        } else {
            tmp2 = RandomGen<T>();
        }
        // Deduced conflicting types for parameter

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        auto compare = [](T a, T b) -> int32_t { return CC == Condition::TST_EQ ? (a & b) == 0 : (a & b) != 0; };

        // Main check - compare parameter and
        // return value
        if (!test->CallCode<T, int32_t>(tmp1, tmp2, compare(tmp1, tmp2))) {
            return false;
        }
    }
    return true;
}

TEST_F(Encoder64Test, JumpTestTest)
{
    EXPECT_TRUE((TestJumpTest<int32_t, Condition::TST_EQ, false>(this)));
    EXPECT_TRUE((TestJumpTest<int32_t, Condition::TST_NE, false>(this)));
    EXPECT_TRUE((TestJumpTest<int64_t, Condition::TST_EQ, false>(this)));
    EXPECT_TRUE((TestJumpTest<int64_t, Condition::TST_NE, false>(this)));
    EXPECT_TRUE((TestJumpTest<uint32_t, Condition::TST_EQ, false>(this)));
    EXPECT_TRUE((TestJumpTest<uint32_t, Condition::TST_NE, false>(this)));
    EXPECT_TRUE((TestJumpTest<uint64_t, Condition::TST_EQ, false>(this)));
    EXPECT_TRUE((TestJumpTest<uint64_t, Condition::TST_NE, false>(this)));

    EXPECT_TRUE((TestJumpTest<int32_t, Condition::TST_EQ, true>(this)));
    EXPECT_TRUE((TestJumpTest<int32_t, Condition::TST_NE, true>(this)));
    EXPECT_TRUE((TestJumpTest<int64_t, Condition::TST_EQ, true>(this)));
    EXPECT_TRUE((TestJumpTest<int64_t, Condition::TST_NE, true>(this)));
    EXPECT_TRUE((TestJumpTest<uint32_t, Condition::TST_EQ, true>(this)));
    EXPECT_TRUE((TestJumpTest<uint32_t, Condition::TST_NE, true>(this)));
    EXPECT_TRUE((TestJumpTest<uint64_t, Condition::TST_EQ, true>(this)));
    EXPECT_TRUE((TestJumpTest<uint64_t, Condition::TST_NE, true>(this)));
}
// NOLINTEND(readability-magic-numbers)

}  // namespace panda::compiler
