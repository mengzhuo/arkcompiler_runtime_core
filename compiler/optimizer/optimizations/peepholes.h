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

#ifndef COMPILER_OPTIMIZER_OPTIMIZATIONS_PEEPHOLES_H_
#define COMPILER_OPTIMIZER_OPTIMIZATIONS_PEEPHOLES_H_

#include "optimizer/ir/graph.h"
#include "optimizer/pass.h"

#include "compiler_logger.h"
#include "optimizer/ir/analysis.h"
#include "optimizer/ir/graph_visitor.h"

namespace panda::compiler {
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class Peepholes : public Optimization, public GraphVisitor {
    using Optimization::Optimization;

public:
    explicit Peepholes(Graph *graph) : Optimization(graph) {}

    NO_MOVE_SEMANTIC(Peepholes);
    NO_COPY_SEMANTIC(Peepholes);
    ~Peepholes() override = default;

    bool RunImpl() override;

    const char *GetPassName() const override
    {
        return "Peepholes";
    }

    bool IsApplied() const
    {
        return is_applied_;
    }

    void InvalidateAnalyses() override;

    const ArenaVector<BasicBlock *> &GetBlocksToVisit() const override
    {
        return GetGraph()->GetBlocksRPO();
    }

    static void VisitSafePoint([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitNeg([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitAbs([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitNot([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitAdd([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitSub([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitMulOneConst([[maybe_unused]] GraphVisitor *v, Inst *inst, Inst *input0, Inst *input1);
    static void VisitMul([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitDiv([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitMin([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitMax([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitMod([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitShl([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitShr([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitAShr([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitAnd([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitOr([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitXor([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitCmp(GraphVisitor *v, Inst *inst);
    static void VisitCompare([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitIf(GraphVisitor *v, Inst *inst);
    static void VisitCast([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitLenArray(GraphVisitor *v, Inst *inst);
    static void VisitPhi([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitSqrt([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitIsInstance(GraphVisitor *v, Inst *inst);
    static void VisitCastValueToAnyType(GraphVisitor *v, Inst *inst);
    static void VisitCompareAnyType(GraphVisitor *v, Inst *inst);
    static void VisitStore([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitStoreObject([[maybe_unused]] GraphVisitor *v, Inst *inst);
    static void VisitStoreStatic([[maybe_unused]] GraphVisitor *v, Inst *inst);

#include "optimizer/ir/visitor.inc"

private:
    void SetIsApplied(Inst *inst, bool altFormat = false, const char *file = nullptr, int line = 0)
    {
        is_applied_ = true;
        if (altFormat) {
            COMPILER_LOG(DEBUG, PEEPHOLE) << "Peephole (" << file << ":" << line << ") is applied for " << *inst;
        } else {
            COMPILER_LOG(DEBUG, PEEPHOLE) << "Peephole is applied for " << GetOpcodeString(inst->GetOpcode());
        }
        inst->GetBasicBlock()->GetGraph()->GetEventWriter().EventPeephole(GetOpcodeString(inst->GetOpcode()),
                                                                          inst->GetId(), inst->GetPc());
        if (options.IsCompilerDumpPeepholes()) {
            std::string name(GetPassName());
            name += "_" + std::to_string(apply_index_);
            GetGraph()->GetPassManager()->DumpGraph(name.c_str());
            apply_index_++;
        }
    }

    // This function check that we can merge two Phi instructions in one basic block.
    static bool IsPhiUnionPossible(PhiInst *phi1, PhiInst *phi2);

    // Get power of 2
    // if n not power of 2 return -1;
    static int64_t GetPowerOfTwo(uint64_t n);

    // Create new instruction instead of current inst
    static Inst *CreateAndInsertInst(Opcode new_opc, Inst *curr_inst, Inst *input0, Inst *input1 = nullptr);

    // Try put constant in second input
    void TrySwapInputs(Inst *inst);

    // Try to remove redundant cast
    void TrySimplifyCastToOperandsType(Inst *inst);
    void TrySimplifyShifts(Inst *inst);
    bool TrySimplifyAddSubWithConstInput(Inst *inst);
    template <Opcode opc, int idx>
    void TrySimplifyAddSub(Inst *inst, Inst *input0, Inst *input1);
    bool TrySimplifyAddSubAdd(Inst *inst, Inst *input0, Inst *input1);
    bool TrySimplifyAddSubSub(Inst *inst, Inst *input0, Inst *input1);
    bool TrySimplifySubAddAdd(Inst *inst, Inst *input0, Inst *input1);
    bool TrySimplifyShlShlAdd(Inst *inst);
    bool TryReassociateShlShlAddSub(Inst *inst);
    void TrySimplifyNeg(Inst *inst);
    void TryReplaceDivByShrAndAshr(Inst *inst, uint64_t unsigned_val, Inst *input0);
    void TryReplaceDivByShift(Inst *inst);
    bool TrySimplifyCompareCaseInputInv(Inst *inst, Inst *input);
    bool TrySimplifyCompareWithBoolInput(Inst *inst);
    bool TrySimplifyCmpCompareWithZero(Inst *inst);
    bool TrySimplifyTestEqualInputs(Inst *inst);
    static bool TrySimplifyCompareAndZero(Inst *inst);
    static bool TrySimplifyCompareAnyType(Inst *inst);
    static bool TrySimplifyCompareLenArrayWithZero(Inst *inst);
    // Try to combine constants when arithmetic operations with constants are repeated
    template <typename T>
    static bool TryCombineConst(Inst *inst, ConstantInst *cnst1, T combine);
    static bool TryCombineAddSubConst(Inst *inst, ConstantInst *cnst1);
    static bool TryCombineShiftConst(Inst *inst, ConstantInst *cnst1);
    static bool TryCombineMulConst(Inst *inst, ConstantInst *cnst1);

    static bool GetInputsOfCompareWithConst(const Inst *inst, Inst **input, ConstantInst **constInput,
                                            bool *inputsSwapped);

    // Table for eliminating 'Compare <CC> <INPUT>, <CONST>'
    enum InputCode {
        INPUT_TRUE,   // Result of compare is TRUE whatever the INPUT is
        INPUT_ORIG,   // Result of compare is equal to the INPUT itself
        INPUT_INV,    // Result of compare is a negation of the INPUT
        INPUT_FALSE,  // Result of compare is FALSE whatever the INPUT is
        INPUT_UNDEF   // Result of compare is undefined
    };
    // clang-format off
    static constexpr std::array<std::array<InputCode, 4>, CC_LAST + 1> VALUES = {{
        /* CONST < 0  CONST == 0   CONST == 1   CONST > 1        CC    */
        {INPUT_FALSE, INPUT_INV,   INPUT_ORIG,  INPUT_FALSE}, /* CC_EQ */
        {INPUT_TRUE,  INPUT_ORIG,  INPUT_INV,   INPUT_TRUE},  /* CC_NE */
        {INPUT_FALSE, INPUT_FALSE, INPUT_INV,   INPUT_TRUE},  /* CC_LT */
        {INPUT_FALSE, INPUT_INV,   INPUT_TRUE,  INPUT_TRUE},  /* CC_LE */
        {INPUT_TRUE,  INPUT_ORIG,  INPUT_FALSE, INPUT_FALSE}, /* CC_GT */
        {INPUT_TRUE,  INPUT_TRUE,  INPUT_ORIG,  INPUT_FALSE}, /* CC_GE */
        /* For unsigned comparison 'CONST < 0' equals to 'CONST > 1'   */
        {INPUT_TRUE,  INPUT_FALSE, INPUT_INV,   INPUT_TRUE},  /* CC_B  */
        {INPUT_TRUE,  INPUT_INV,   INPUT_TRUE,  INPUT_TRUE},  /* CC_BE */
        {INPUT_FALSE, INPUT_ORIG,  INPUT_FALSE, INPUT_FALSE}, /* CC_A  */
        {INPUT_FALSE, INPUT_TRUE,  INPUT_ORIG,  INPUT_FALSE}  /* CC_AE */
    }};
    // clang-format on
    static InputCode GetInputCode(const ConstantInst *inst, ConditionCode cc)
    {
        if (inst->GetType() == DataType::INT32) {
            auto i = std::min<int32_t>(std::max<int32_t>(-1, static_cast<int32_t>(inst->GetInt32Value())), 2U) + 1;
            return VALUES[cc][i];
        }
        if (inst->GetType() == DataType::INT64) {
            auto i = std::min<int64_t>(std::max<int64_t>(-1, static_cast<int64_t>(inst->GetIntValue())), 2U) + 1;
            return VALUES[cc][i];
        }
        UNREACHABLE();
        return InputCode::INPUT_UNDEF;
    }
    template <typename T>
    static void EliminateInstPrecedingStore(GraphVisitor *v, Inst *inst);

private:
    // Each peephole application has own unique index, it will be used in peepholes dumps
    uint32_t apply_index_ {0};

    bool is_applied_ {false};
};
}  // namespace panda::compiler

#endif  // COMPILER_OPTIMIZER_OPTIMIZATIONS_PEEPHOLES_H_
