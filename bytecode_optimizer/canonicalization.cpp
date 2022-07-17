/*
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
#include "canonicalization.h"

#include "compiler/optimizer/ir/basicblock.h"
#include "compiler/optimizer/ir/inst.h"

namespace panda::bytecodeopt {

bool Canonicalization::RunImpl()
{
    Canonicalization visitor(GetGraph());
    for (auto bb : GetGraph()->GetBlocksRPO()) {
        for (auto inst : bb->AllInsts()) {
            if (inst->IsCommutative()) {
                visitor.VisitCommutative(inst);
            } else {
                visitor.VisitInstruction(inst);
            }
        }
    }
    return visitor.GetStatus();
}

static bool IsDominateReverseInputs(const compiler::Inst *inst)
{
    auto input0 = inst->GetInput(0).GetInst();
    auto input1 = inst->GetInput(1).GetInst();
    return input0->IsDominate(input1);
}

static bool ConstantFitsCompareImm(const Inst *cst, uint32_t size)
{
    ASSERT(cst->GetOpcode() == Opcode::Constant);
    if (compiler::DataType::IsFloatType(cst->GetType())) {
        return false;
    }
    auto val = cst->CastToConstant()->GetIntValue();
    return (size == compiler::HALF_SIZE) && (val == 0);
}

static bool BetterToSwapCompareInputs(const compiler::Inst *inst, const compiler::Inst *input0,
                                      const compiler::Inst *input1)
{
    if (!input0->IsConst()) {
        return false;
    }
    if (!input1->IsConst()) {
        return true;
    }

    compiler::DataType::Type type = inst->CastToCompare()->GetOperandsType();
    uint32_t size = (type == compiler::DataType::UINT64 || type == compiler::DataType::INT64) ? compiler::WORD_SIZE
                                                                                              : compiler::HALF_SIZE;
    return ConstantFitsCompareImm(input0, size) && !ConstantFitsCompareImm(input1, size);
}

static bool SwapInputsIfNecessary(compiler::Inst *inst, const bool necessary)
{
    if (!necessary) {
        return false;
    }
    auto input0 = inst->GetInput(0).GetInst();
    auto input1 = inst->GetInput(1).GetInst();
    if ((inst->GetOpcode() == compiler::Opcode::Compare) && !BetterToSwapCompareInputs(inst, input0, input1)) {
        return false;
    }

    inst->SwapInputs();
    return true;
}

bool Canonicalization::TrySwapConstantInput(Inst *inst)
{
    return SwapInputsIfNecessary(inst, inst->GetInput(0).GetInst()->IsConst());
}

bool Canonicalization::TrySwapReverseInput(Inst *inst)
{
    return SwapInputsIfNecessary(inst, IsDominateReverseInputs(inst));
}

void Canonicalization::VisitCommutative(Inst *inst)
{
    ASSERT(inst->IsCommutative());
    ASSERT(inst->GetInputsCount() == 2);  // 2 is COMMUTATIVE_INPUT_COUNT
    if (options.GetOptLevel() > 1) {
        result_ = TrySwapReverseInput(inst);
    }
    result_ = TrySwapConstantInput(inst) || result_;
}

// It is not allowed to move a constant input1 with a single user (it's processed Compare instruction).
// This is necessary for further merging of the constant and the If instrution in the Lowering pass
bool AllowSwap(const compiler::Inst *inst)
{
    auto input1 = inst->GetInput(1).GetInst();
    if (!input1->IsConst()) {
        return true;
    }
    for (const auto &user : input1->GetUsers()) {
        if (user.GetInst() != inst) {
            return true;
        }
    }
    return false;
}

void Canonicalization::VisitCompare([[maybe_unused]] GraphVisitor *v, Inst *inst_base)
{
    auto inst = inst_base->CastToCompare();
    if (AllowSwap(inst) && SwapInputsIfNecessary(inst, IsDominateReverseInputs(inst))) {
        auto revert_cc = SwapOperandsConditionCode(inst->GetCc());
        inst->SetCc(revert_cc);
    }
}

}  // namespace panda::bytecodeopt
