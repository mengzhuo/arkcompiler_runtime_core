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

#include "check_resolver.h"
#include "compiler/optimizer/ir/basicblock.h"
#include "compiler/optimizer/ir/inst.h"

namespace panda::bytecodeopt {

bool CheckResolver::RunImpl()
{
    bool applied = false;
    for (auto bb : GetGraph()->GetBlocksRPO()) {
        for (auto inst : bb->Insts()) {
            auto op = inst->GetOpcode();

            // replace check
            if (IsCheck(inst)) {
                size_t i = (op == compiler::Opcode::BoundsCheck || op == compiler::Opcode::RefTypeCheck) ? 1 : 0;
                auto input = inst->GetInput(i).GetInst();
                for (auto &user : inst->GetUsers()) {
                    user.GetInst()->SetFlag(compiler::inst_flags::CAN_THROW);
                }
                inst->ReplaceUsers(input);
                inst->ClearFlag(compiler::inst_flags::NO_DCE);  // DCE will remove the check inst
                applied = true;
            }

            // set NO_DCE for Div, Mod, LoadArray, LoadStatic and LoadObject
            if (NeedSetNoDCE(inst)) {
                inst->SetFlag(compiler::inst_flags::NO_DCE);
                applied = true;
            }

            // mark LenArray whose users are not all check as no_dce
            // mark LenArray as no_hoist in all cases
            if (inst->GetOpcode() == compiler::Opcode::LenArray) {
                bool no_dce = !inst->HasUsers();
                for (const auto &usr : inst->GetUsers()) {
                    if (!IsCheck(usr.GetInst())) {
                        no_dce = true;
                        break;
                    }
                }
                if (no_dce) {
                    inst->SetFlag(compiler::inst_flags::NO_DCE);
                }
                inst->SetFlag(compiler::inst_flags::NO_HOIST);
            }
        }
    }
    return applied;
}

bool CheckResolver::NeedSetNoDCE(const compiler::Inst *inst)
{
    switch (inst->GetOpcode()) {
        case compiler::Opcode::Div:
        case compiler::Opcode::Mod:
        case compiler::Opcode::LoadArray:
        case compiler::Opcode::LoadStatic:
        case compiler::Opcode::LoadObject:
            return true;
        default:
            return false;
    }
}

bool CheckResolver::IsCheck(const compiler::Inst *inst)
{
    switch (inst->GetOpcode()) {
        case compiler::Opcode::BoundsCheck:
        case compiler::Opcode::NullCheck:
        case compiler::Opcode::NegativeCheck:
        case compiler::Opcode::ZeroCheck:
        case compiler::Opcode::RefTypeCheck:
        case compiler::Opcode::BoundsCheckI:
            return true;
        default:
            return false;
    }
}

}  // namespace panda::bytecodeopt
