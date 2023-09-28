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

#ifndef PANDA_RUNTIME_TOOLING_SAMPLER_SAMPLE_INFO_H_
#define PANDA_RUNTIME_TOOLING_SAMPLER_SAMPLE_INFO_H_

#include <cstring>
#include <string>
#include <array>

#include "libpandabase/macros.h"
#include "libpandabase/os/thread.h"

namespace panda::tooling::sampler {

// Saving one sample info
struct SampleInfo {
    enum class ThreadStatus : uint32_t { UNDECLARED = 0, RUNNING = 1, SUSPENDED = 2 };

    struct ManagedStackFrameId {
        uintptr_t file_id {0};
        uintptr_t panda_file_ptr {0};
    };

    struct StackInfo {
        static constexpr size_t MAX_STACK_DEPTH = 100;
        uintptr_t managed_stack_size {0};
        // We can't use dynamic memory 'cause of usage inside the signal handler
        std::array<ManagedStackFrameId, MAX_STACK_DEPTH> managed_stack;
    };

    struct ThreadInfo {
        // Id of the thread from which sample was obtained
        uint32_t thread_id {0};
        ThreadStatus thread_status {ThreadStatus::UNDECLARED};
    };

    ThreadInfo thread_info {};
    StackInfo stack_info {};
};

// Saving one module info (panda file, .so)
struct FileInfo {
    uintptr_t ptr {0};
    uint32_t checksum {0};
    std::string pathname;
};

enum class FrameKind : uintptr_t { BRIDGE = 1 };

bool operator==(const SampleInfo &lhs, const SampleInfo &rhs);
bool operator!=(const SampleInfo &lhs, const SampleInfo &rhs);
bool operator==(const FileInfo &lhs, const FileInfo &rhs);
bool operator!=(const FileInfo &lhs, const FileInfo &rhs);
bool operator==(const SampleInfo::ManagedStackFrameId &lhs, const SampleInfo::ManagedStackFrameId &rhs);
bool operator!=(const SampleInfo::ManagedStackFrameId &lhs, const SampleInfo::ManagedStackFrameId &rhs);
bool operator==(const SampleInfo::StackInfo &lhs, const SampleInfo::StackInfo &rhs);
bool operator!=(const SampleInfo::StackInfo &lhs, const SampleInfo::StackInfo &rhs);
bool operator==(const SampleInfo::ThreadInfo &lhs, const SampleInfo::ThreadInfo &rhs);
bool operator!=(const SampleInfo::ThreadInfo &lhs, const SampleInfo::ThreadInfo &rhs);

inline uintptr_t ReadUintptrTBitMisaligned(const void *ptr)
{
    /*
     * Pointer might be misaligned
     * To avoid of UB we should read misaligned adresses with memcpy
     */
    uintptr_t value = 0;
    memcpy(&value, ptr, sizeof(value));
    return value;
}

inline uint32_t ReadUint32TBitMisaligned(const void *ptr)
{
    uint32_t value = 0;
    memcpy(&value, ptr, sizeof(value));
    return value;
}

inline bool operator==(const SampleInfo::ManagedStackFrameId &lhs, const SampleInfo::ManagedStackFrameId &rhs)
{
    return lhs.file_id == rhs.file_id && lhs.panda_file_ptr == rhs.panda_file_ptr;
}

inline bool operator!=(const SampleInfo::ManagedStackFrameId &lhs, const SampleInfo::ManagedStackFrameId &rhs)
{
    return !(lhs == rhs);
}

inline bool operator==(const FileInfo &lhs, const FileInfo &rhs)
{
    return lhs.ptr == rhs.ptr && lhs.pathname == rhs.pathname;
}

inline bool operator!=(const FileInfo &lhs, const FileInfo &rhs)
{
    return !(lhs == rhs);
}

inline bool operator==(const SampleInfo::StackInfo &lhs, const SampleInfo::StackInfo &rhs)
{
    if (lhs.managed_stack_size != rhs.managed_stack_size) {
        return false;
    }
    for (uint32_t i = 0; i < lhs.managed_stack_size; ++i) {
        if (lhs.managed_stack[i] != rhs.managed_stack[i]) {
            return false;
        }
    }
    return true;
}

inline bool operator!=(const SampleInfo::StackInfo &lhs, const SampleInfo::StackInfo &rhs)
{
    return !(lhs == rhs);
}

inline bool operator==(const SampleInfo::ThreadInfo &lhs, const SampleInfo::ThreadInfo &rhs)
{
    return lhs.thread_id == rhs.thread_id && lhs.thread_status == rhs.thread_status;
}

inline bool operator!=(const SampleInfo::ThreadInfo &lhs, const SampleInfo::ThreadInfo &rhs)
{
    return !(lhs == rhs);
}

inline bool operator==(const SampleInfo &lhs, const SampleInfo &rhs)
{
    if (lhs.thread_info != rhs.thread_info) {
        return false;
    }
    if (lhs.stack_info != rhs.stack_info) {
        return false;
    }
    return true;
}

inline bool operator!=(const SampleInfo &lhs, const SampleInfo &rhs)
{
    return !(lhs == rhs);
}

}  // namespace panda::tooling::sampler

// Definind std::hash for SampleInfo to use it as an unordered_map key
namespace std {

template <>
struct hash<panda::tooling::sampler::SampleInfo> {
    std::size_t operator()(const panda::tooling::sampler::SampleInfo &s) const
    {
        auto stack_info = s.stack_info;
        ASSERT(stack_info.managed_stack_size <= panda::tooling::sampler::SampleInfo::StackInfo::MAX_STACK_DEPTH);
        size_t summ = 0;
        for (size_t i = 0; i < stack_info.managed_stack_size; ++i) {
            summ += stack_info.managed_stack[i].panda_file_ptr ^ stack_info.managed_stack[i].file_id;
        }
        constexpr uint32_t THREAD_STATUS_SHIFT = 20;
        return std::hash<size_t>()(
            (summ ^ stack_info.managed_stack_size) +
            (s.thread_info.thread_id ^ (static_cast<uint32_t>(s.thread_info.thread_status) << THREAD_STATUS_SHIFT)));
    }
};

// Definind std::hash for FileInfo to use it as an unordered_set key
template <>
struct hash<panda::tooling::sampler::FileInfo> {
    size_t operator()(const panda::tooling::sampler::FileInfo &m) const
    {
        size_t h1 = std::hash<uintptr_t> {}(m.ptr);
        size_t h2 = std::hash<uint32_t> {}(m.checksum);
        size_t h3 = std::hash<std::string> {}(m.pathname);
        return h1 ^ (h2 << 1U) ^ (h3 << 2U);
    }
};

}  // namespace std

#endif  // PANDA_RUNTIME_TOOLING_SAMPLER_SAMPLE_INFO_H_
