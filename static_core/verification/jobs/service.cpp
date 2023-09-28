/**
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "verification/jobs/service.h"

#include "runtime/mem/allocator_adapter.h"

namespace panda::verifier {

void VerifierService::Init() {}

void VerifierService::Destroy()
{
    panda::os::memory::LockHolder lck {lock_};
    if (shutdown_) {
        return;
    }
    shutdown_ = true;

    for (auto &it : processors_) {
        auto *lang_data = &it.second;
        // Wait for ongoing verifications to finish
        while (lang_data->total_processors < lang_data->queue.size()) {
            cond_var_.Wait(&lock_);
        }
        for (auto *processor : lang_data->queue) {
            allocator_->Delete<TaskProcessor>(processor);
        }
    }
}

TaskProcessor *VerifierService::GetProcessor(SourceLang lang)
{
    panda::os::memory::LockHolder lck {lock_};
    if (shutdown_) {
        return nullptr;
    }
    if (processors_.count(lang) == 0) {
        processors_.emplace(lang, lang);
    }
    LangData *lang_data = &processors_.at(lang);
    if (lang_data->queue.empty()) {
        lang_data->queue.push_back(allocator_->New<TaskProcessor>(this, lang));
        lang_data->total_processors++;
    }
    // TODO(gogabr): should we use a queue or stack discipline?
    auto res = lang_data->queue.front();
    lang_data->queue.pop_front();
    return res;
}

void VerifierService::ReleaseProcessor(TaskProcessor *processor)
{
    panda::os::memory::LockHolder lck {lock_};
    auto lang = processor->GetLang();
    ASSERT(processors_.count(lang) > 0);
    processors_.at(lang).queue.push_back(processor);
    cond_var_.Signal();
}

}  // namespace panda::verifier
