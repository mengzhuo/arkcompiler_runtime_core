/**
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "abc_code_converter.h"
#include <iostream>
#include "abc_type_converter.h"
#include "abc_file_utils.h"

namespace panda::abc2program {

AbcCodeConverter::AbcCodeConverter(Abc2ProgramEntityContainer &entity_container) : entity_container_(entity_container)
{
    file_ = &(entity_container_.GetAbcFile());
    string_table_ = &(entity_container_.GetAbcStringTable());
}

std::string AbcCodeConverter::IDToString(BytecodeInstruction bc_ins, panda_file::File::EntityId method_id,
                                         size_t idx) const
{
    std::stringstream name;
    panda_file::File::EntityId entity_id = file_->ResolveOffsetByIndex(method_id, bc_ins.GetId(idx).AsIndex());
    if (bc_ins.IsIdMatchFlag(idx, BytecodeInstruction::Flags::METHOD_ID)) {
        std::string full_method_name = entity_container_.GetFullMethodNameById(entity_id);
        entity_container_.AddProgramString(full_method_name);
        name << full_method_name;
    } else if (bc_ins.IsIdMatchFlag(idx, BytecodeInstruction::Flags::STRING_ID)) {
        string_table_->AddStringId(entity_id);
        std::string str_constant = string_table_->GetStringById(entity_id);
        entity_container_.AddProgramString(str_constant);
        name << str_constant;
    } else {
        ASSERT(bc_ins.IsIdMatchFlag(idx, BytecodeInstruction::Flags::LITERALARRAY_ID));
        entity_container_.AddLiteralArrayId(entity_id);
        name << entity_container_.GetLiteralArrayIdName(entity_id);
    }
    return name.str();
}

}  // namespace panda::abc2program