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

#include "runtime/include/gc_task.h"

#include "runtime/mem/gc/gc.h"

namespace panda {

std::atomic<uint32_t> GCTask::next_id_ = 1;

void GCTask::Run(mem::GC &gc)
{
    gc.WaitForGC(*this);
    gc.SetCanAddGCTask(true);
}

void GCTask::Release(mem::InternalAllocatorPtr allocator)
{
    allocator->Delete(this);
}

void GCTask::UpdateGCCollectionType(GCCollectionType gc_collection_type)
{
    ASSERT(gc_collection_type != GCCollectionType::NONE);
    if (gc_collection_type <= collection_type) {
        return;
    }
    collection_type = gc_collection_type;
}

std::ostream &operator<<(std::ostream &os, const GCTaskCause &cause)
{
    switch (cause) {
        case GCTaskCause::INVALID_CAUSE:
            os << "Invalid";
            break;
        case GCTaskCause::PYGOTE_FORK_CAUSE:
            os << "PygoteFork";
            break;
        case GCTaskCause::STARTUP_COMPLETE_CAUSE:
            os << "StartupComplete";
            break;
        case GCTaskCause::NATIVE_ALLOC_CAUSE:
            os << "NativeAlloc";
            break;
        case GCTaskCause::EXPLICIT_CAUSE:
            os << "Explicit";
            break;
        case GCTaskCause::HEAP_USAGE_THRESHOLD_CAUSE:
            os << "Threshold";
            break;
        case GCTaskCause::MIXED:
            os << "Mixed";
            break;
        case GCTaskCause::YOUNG_GC_CAUSE:
            os << "Young";
            break;
        case GCTaskCause::OOM_CAUSE:
            os << "OOM";
            break;
        default:
            LOG(FATAL, GC) << "Unknown gc cause";
            break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const GCCollectionType &collection_type)
{
    switch (collection_type) {
        case GCCollectionType::NONE:
            os << "NONE";
            break;
        case GCCollectionType::YOUNG:
            os << "YOUNG";
            break;
        case GCCollectionType::TENURED:
            os << "TENURED";
            break;
        case GCCollectionType::MIXED:
            os << "MIXED";
            break;
        case GCCollectionType::FULL:
            os << "FULL";
            break;
        default:
            LOG(FATAL, GC) << "Unknown collection type";
            break;
    }
    return os;
}

}  // namespace panda
