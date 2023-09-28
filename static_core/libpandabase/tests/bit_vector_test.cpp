/**
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "utils/bit_vector.h"
#include "mem/pool_manager.h"
#include "utils/arena_containers.h"

#include <gtest/gtest.h>

namespace panda::test {

// NOLINTBEGIN(readability-magic-numbers)

class BitVectorTest : public ::testing::Test {
public:
    NO_MOVE_SEMANTIC(BitVectorTest);
    NO_COPY_SEMANTIC(BitVectorTest);

    BitVectorTest()
    {
        panda::mem::MemConfig::Initialize(0, 64_MB, 256_MB, 32_MB, 0, 0);
        PoolManager::Initialize();
        allocator_ = new ArenaAllocator(SpaceType::SPACE_TYPE_COMPILER);
    }

    ~BitVectorTest() override
    {
        delete allocator_;
        PoolManager::Finalize();
        panda::mem::MemConfig::Finalize();
    }

    ArenaAllocator *GetAllocator()
    {
        return allocator_;
    }

private:
    ArenaAllocator *allocator_ {nullptr};
};

TEST_F(BitVectorTest, Basics)
{
    BitVector<> vector;
    const BitVector<> &cvector = vector;
    ASSERT_EQ(vector.capacity(), 0);

    // Index iterators for empty vector
    for (uint32_t i : cvector.GetSetBitsIndices()) {
        (void)i;
        ADD_FAILURE();
    }
    for (uint32_t i : cvector.GetZeroBitsIndices()) {
        (void)i;
        ADD_FAILURE();
    }

    vector.push_back(true);
    vector.push_back(false);
    ASSERT_NE(vector.capacity(), 0);

    // Check `GetDataSpan`
    ASSERT_NE(vector.GetDataSpan().size(), 0);
    uint32_t value = 1;
    ASSERT_EQ(std::memcmp(vector.GetDataSpan().data(), &value, 1), 0);

    // Constant operator[]
    ASSERT_EQ(cvector[0], vector[0]);

    // Constant versions of begin and end
    ASSERT_EQ(cvector.begin(), vector.begin());
    ASSERT_EQ(cvector.end(), vector.end());

    vector.resize(20);
    std::fill(vector.begin(), vector.end(), false);
    ASSERT_EQ(vector.PopCount(), 0);
    std::fill(vector.begin() + 2, vector.begin() + 15, true);
    ASSERT_EQ(vector.PopCount(), 13);
    for (size_t i = 0; i < 15; i++) {
        if (i > 2) {
            ASSERT_EQ(vector.PopCount(i), i - 2);
        } else {
            ASSERT_EQ(vector.PopCount(i), 0);
        }
    }
    ASSERT_EQ(vector.GetHighestBitSet(), 14);
    ASSERT_EQ(vector[0], false);
    ASSERT_EQ(vector[1], false);
    ASSERT_EQ(vector[2], true);

    ASSERT_EQ(vector, vector.GetFixed());
    ASSERT_FALSE(vector.GetContainerDataSpan().empty());
}

TEST_F(BitVectorTest, Comparison)
{
    std::vector<bool> values = {false, true, false, true, false, true, false, true, false, true};
    BitVector<> vec1;
    std::copy(values.begin(), values.end(), std::back_inserter(vec1));
    BitVector<ArenaAllocator> vec2(GetAllocator());
    std::copy(values.begin(), values.end(), std::back_inserter(vec2));
    ASSERT_EQ(vec1, vec2);
    vec2[0] = true;
    ASSERT_NE(vec1, vec2);
}

template <typename T>
void TestIteration(T &vector, size_t bits)
{
    int index = 0;

    ASSERT_FALSE(vector.empty());
    ASSERT_EQ(vector.size(), bits);

    std::fill(vector.begin(), vector.end(), true);
    for (uint32_t i : vector.GetZeroBitsIndices()) {
        ADD_FAILURE();
    }
    index = 0;
    for (uint32_t i : vector.GetSetBitsIndices()) {
        ASSERT_EQ(i, index++);
    }

    std::fill(vector.begin(), vector.end(), false);
    for (uint32_t i : vector.GetSetBitsIndices()) {
        ADD_FAILURE();
    }
    index = 0;
    for (uint32_t i : vector.GetZeroBitsIndices()) {
        ASSERT_EQ(i, index++);
    }

    index = 0;
    for (auto v : vector) {
        v = (index++ % 2) != 0;
    }
    index = 0;
    for (auto v : vector) {
        ASSERT_EQ(v, index++ % 2);
    }
    index = vector.size() - 1;
    for (auto it = vector.end() - 1;; --it) {
        ASSERT_EQ(*it, index-- % 2);
        if (it == vector.begin()) {
            break;
        }
    }
    index = 1;
    for (uint32_t i : vector.GetSetBitsIndices()) {
        ASSERT_EQ(i, index);
        index += 2;
    }
    index = 0;
    for (uint32_t i : vector.GetZeroBitsIndices()) {
        ASSERT_EQ(i, index);
        index += 2;
    }

    auto it = vector.begin();
    ASSERT_EQ(*it, false);
    ++it;
    ASSERT_EQ(*it, true);
    auto it1 = it++;
    ASSERT_EQ(*it, false);
    ASSERT_EQ(*it1, true);
    ASSERT_TRUE(it1 < it);
    it += 3;
    ASSERT_EQ(*it, true);
    it -= 5;
    ASSERT_EQ(*it, false);
    ASSERT_EQ(it, vector.begin());

    it = it + 6;
    ASSERT_EQ(*it, false);
    ASSERT_EQ(std::distance(vector.begin(), it), 6);
    ASSERT_EQ(it[1], true);
    it = it - 3;
    ASSERT_EQ(*it, true);
    ASSERT_EQ(std::distance(vector.begin(), it), 3);
    --it;
    ASSERT_EQ(*it, false);
    it1 = it--;
    ASSERT_EQ(*it, true);
    ASSERT_EQ(*it1, false);
    ASSERT_TRUE(it1 > it);
    it = vector.begin() + 100;
    ASSERT_EQ(std::distance(vector.begin(), it), 100);
    ASSERT_TRUE(it + 2 > it);
    ASSERT_TRUE(it + 2 >= it);
    ASSERT_TRUE(it + 0 >= it);
    ASSERT_TRUE(it - 2 < it);
    ASSERT_TRUE(it - 2 <= it);

    auto cit = vector.cbegin();
    ASSERT_EQ(cit, vector.begin());
    ASSERT_EQ(++cit, ++vector.begin());
    ASSERT_EQ(vector.cend(), vector.end());
}

TEST_F(BitVectorTest, Iteration)
{
    std::array<uint32_t, 10> data {};
    size_t bits_num = data.size() * BitsNumInValue(data[0]);

    BitVector<> vec1;
    vec1.resize(bits_num);
    TestIteration(vec1, bits_num);

    BitVector<ArenaAllocator> vec2(GetAllocator());
    vec2.resize(bits_num);
    TestIteration(vec2, bits_num);

    BitVector<ArenaAllocator> vec3(bits_num, GetAllocator());
    TestIteration(vec3, bits_num);

    BitVectorSpan vec4(Span<uint32_t>(data.data(), data.size()));
    TestIteration(vec4, bits_num);

    data.fill(0);
    BitVectorSpan vec5(data.data(), bits_num);
    TestIteration(vec5, bits_num);
}

template <typename T>
void TestModification(T &vector)
{
    std::vector<bool> values = {false, true, false, true, false, true, false, true, false, true};
    ASSERT_TRUE(vector.empty());
    ASSERT_EQ(vector.size(), 0);
    ASSERT_EQ(vector.PopCount(), 0);
    ASSERT_EQ(vector.GetHighestBitSet(), -1);

    vector.push_back(true);
    ASSERT_FALSE(vector.empty());
    ASSERT_EQ(vector.size(), 1);
    ASSERT_EQ(vector.PopCount(), 1);
    ASSERT_EQ(vector.GetHighestBitSet(), 0);

    std::copy(values.begin(), values.end(), std::back_inserter(vector));
    ASSERT_EQ(vector.size(), 11);
    ASSERT_EQ(vector[1], false);
    ASSERT_EQ(vector.PopCount(), 6);
    ASSERT_EQ(vector.GetHighestBitSet(), 10);

    vector[1] = true;
    ASSERT_EQ(vector[1], true);

    uint32_t value = 0b10101010111;
    ASSERT_EQ(std::memcmp(vector.data(), &value, vector.GetSizeInBytes()), 0);

    vector.resize(3);
    ASSERT_EQ(vector.size(), 3);
    ASSERT_EQ(vector.PopCount(), 3);

    vector.resize(10);
    ASSERT_EQ(vector.PopCount(), 3);

    vector.clear();
    ASSERT_TRUE(vector.empty());
    ASSERT_EQ(vector.size(), 0);

    // Push 1000 values with `true` in odd and `false` in even indexes
    for (int i = 0; i < 100; i++) {
        std::copy(values.begin(), values.end(), std::back_inserter(vector));
    }
    ASSERT_EQ(vector.size(), 1000);
    ASSERT_EQ(vector.PopCount(), 500);
    for (int i = 0; i < 1000; i++) {
        vector.push_back(false);
    }
    ASSERT_EQ(vector.size(), 2000);
    ASSERT_EQ(vector.PopCount(), 500);
    ASSERT_EQ(vector.GetHighestBitSet(), 999);

    vector.ClearBit(3000);
    ASSERT_EQ(vector.size(), 3001);
    ASSERT_EQ(vector.PopCount(), 500);
    ASSERT_EQ(vector.GetHighestBitSet(), 999);

    vector.SetBit(4000);
    ASSERT_EQ(vector.size(), 4001);
    ASSERT_EQ(vector.PopCount(), 501);
    ASSERT_EQ(vector.GetHighestBitSet(), 4000);
}

TEST_F(BitVectorTest, Modification)
{
    BitVector<> vec1;
    TestModification(vec1);
    BitVector<ArenaAllocator> vec2(GetAllocator());
    TestModification(vec2);
}

TEST_F(BitVectorTest, SetClearBit)
{
    BitVector<> vector;

    vector.SetBit(55);
    ASSERT_EQ(vector.size(), 56);

    vector.SetBit(45);
    ASSERT_EQ(vector.size(), 56);
    ASSERT_EQ(vector.PopCount(), 2);

    vector.ClearBit(105);
    ASSERT_EQ(vector.size(), 106);
    ASSERT_EQ(vector.PopCount(), 2);
    ASSERT_EQ(vector.GetHighestBitSet(), 55);

    vector.ClearBit(45);
    ASSERT_EQ(vector.size(), 106);
    ASSERT_EQ(vector.PopCount(), 1);
}

TEST_F(BitVectorTest, TestUnion)
{
    {
        BitVector<> vector0;
        BitVector<> vector1;

        vector0 |= vector1;
        ASSERT_TRUE(vector0.empty());
    }

    {
        BitVector<> vector0;
        vector0.SetBit(13);
        vector0.SetBit(71);

        BitVector<> vector1;
        vector1.SetBit(10);
        vector1.SetBit(13);
        vector1.SetBit(42);
        vector1.SetBit(77);

        vector0 |= vector1;
        ASSERT_EQ(vector0.size(), 78);
        ASSERT_EQ(vector0.PopCount(), 5);
        ASSERT_TRUE(vector0.GetBit(10));
        ASSERT_TRUE(vector0.GetBit(13));
        ASSERT_TRUE(vector0.GetBit(42));
        ASSERT_TRUE(vector0.GetBit(71));
        ASSERT_TRUE(vector0.GetBit(77));

        ASSERT_EQ(vector1.PopCount(), 4);
    }

    {
        BitVector<> vector0;
        vector0.SetBit(1);

        BitVector<> vector1;
        vector1.SetBit(128);

        vector0 |= vector1;

        ASSERT_EQ(vector0.size(), 129);
        ASSERT_EQ(vector0.PopCount(), 2);
        ASSERT_TRUE(vector0.GetBit(1));
        ASSERT_TRUE(vector0.GetBit(128));
    }

    {
        BitVector<> vector0;
        vector0.SetBit(128);

        BitVector<> vector1;
        vector1.SetBit(1);

        vector0 |= vector1;

        ASSERT_EQ(vector0.size(), 129);
        ASSERT_EQ(vector0.PopCount(), 2);
        ASSERT_TRUE(vector0.GetBit(1));
        ASSERT_TRUE(vector0.GetBit(128));
    }
}

// NOLINTEND(readability-magic-numbers)

}  // namespace panda::test
