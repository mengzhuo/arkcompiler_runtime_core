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

/*
Register file implementation.
Reserve registers.
*/

#include "registers_description.h"
#include "target/aarch32/target.h"
#include "regfile.h"

namespace panda::compiler::aarch32 {
/**
 * Default aarch32 calling convention registers
 * Callee
 *    r4-r11,r14
 *    d8-d15
 * Caller
 *    (r0-r3),r12
 *     d0-d7
 */
Aarch32RegisterDescription::Aarch32RegisterDescription(ArenaAllocator *allocator)
    : RegistersDescription(allocator, Arch::AARCH32),
      aarch32RegList_(allocator->Adapter()),
      usedRegs_(allocator->Adapter())
{
    // Initialize Masm
    for (uint32_t i = 0; i <= MAX_NUM_REGS; ++i) {
        aarch32RegList_.emplace_back(Reg(i, INT32_TYPE));
        aarch32RegList_.emplace_back(Reg(i, FLOAT32_TYPE));
    }

    for (auto i = vixl::aarch32::r4.GetCode(); i < vixl::aarch32::r8.GetCode(); ++i) {
        callerSavedv_.set(i);
    }
}

bool Aarch32RegisterDescription::IsValid() const
{
    return !aarch32RegList_.empty();
}

bool Aarch32RegisterDescription::IsRegUsed(ArenaVector<Reg> vecReg, Reg reg)
{
    auto equality = [reg](Reg in) { return (reg.GetId() == in.GetId()) && (reg.GetType() == in.GetType()); };
    return (std::find_if(vecReg.begin(), vecReg.end(), equality) != vecReg.end());
}

/* static */
bool Aarch32RegisterDescription::IsTmp(Reg reg)
{
    if (reg.IsScalar()) {
        for (auto it : AARCH32_TMP_REG) {
            if (it == reg.GetId()) {
                return true;
            }
        }
        return false;
    }
    ASSERT(reg.IsFloat());
    for (auto it : AARCH32_TMP_VREG) {
        if (it == reg.GetId()) {
            return true;
        }
    }
    return false;
}

ArenaVector<Reg> Aarch32RegisterDescription::GetCalleeSaved()
{
    ArenaVector<Reg> out(GetAllocator()->Adapter());
    ASSERT(calleeSaved_.size() == calleeSavedv_.size());
    for (size_t i = 0; i < calleeSaved_.size(); ++i) {
        if (calleeSaved_.test(i)) {
            out.emplace_back(Reg(i, INT32_TYPE));
        }
        if ((calleeSavedv_.test(i))) {
            out.emplace_back(Reg(i, FLOAT32_TYPE));
        }
    }
    return out;
}

void Aarch32RegisterDescription::SetCalleeSaved([[maybe_unused]] const ArenaVector<Reg> &regs)
{
    calleeSaved_ = CALLEE_SAVED;
    calleeSavedv_ = CALLEE_SAVEDV;
}

void Aarch32RegisterDescription::SetUsedRegs(const ArenaVector<Reg> &regs)
{
    usedRegs_ = regs;

    ASSERT(calleeSaved_.size() == callerSaved_.size());
    ASSERT(calleeSavedv_.size() == callerSavedv_.size());

    allignmentRegCallee_ = vixl::aarch32::r10.GetCode();
    // NOTE (pishin) need to resolve conflict
    allignmentRegCaller_ = vixl::aarch32::r10.GetCode();
    for (size_t i = 0; i < calleeSaved_.size(); ++i) {
        // IsRegUsed use used_regs_ variable
        bool scalarUsed = IsRegUsed(usedRegs_, Reg(i, INT64_TYPE));
        bool isTmp = IsTmp(Reg(i, INT32_TYPE));
        if ((!scalarUsed && ((calleeSaved_.test(i)))) || isTmp) {
            calleeSaved_.reset(i);
            allignmentRegCallee_ = i;
        }
        if (!scalarUsed && ((callerSaved_.test(i)))) {
            allignmentRegCaller_ = i;
        }
        bool isVtmp = IsTmp(Reg(i, FLOAT32_TYPE));

        bool vectorUsed = IsRegUsed(usedRegs_, Reg(i, FLOAT64_TYPE));
        if ((!vectorUsed && ((calleeSavedv_.test(i)))) || isVtmp) {
            calleeSavedv_.reset(i);
        }
        if (!vectorUsed && ((callerSavedv_.test(i)))) {
            callerSavedv_.reset(i);
        }
        if (i > (AVAILABLE_DOUBLE_WORD_REGISTERS << 1U)) {
            continue;
        }
        if (!scalarUsed && ((calleeSaved_.test(i + 1)))) {
            calleeSaved_.reset(i + 1);
        }
    }

    calleeSaved_.reset(vixl::aarch32::pc.GetCode());
    callerSaved_.reset(vixl::aarch32::pc.GetCode());
}

}  // namespace panda::compiler::aarch32
