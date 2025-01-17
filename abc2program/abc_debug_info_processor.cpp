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

#include "abc_debug_info_processor.h"
#include "abc2program_log.h"

namespace panda::abc2program {

AbcDebugInfoProcessor::AbcDebugInfoProcessor(panda_file::File::EntityId entity_id,
                                             Abc2ProgramEntityContainer &entity_container)
    : AbcFileEntityProcessor(entity_id, entity_container)
{
    debug_info_data_accessor_ = std::make_unique<panda_file::DebugInfoDataAccessor>(*file_, entity_id_);
}

void AbcDebugInfoProcessor::FillProgramData()
{
    log::Unimplemented(__PRETTY_FUNCTION__);
}

} // namespace panda::abc2program
