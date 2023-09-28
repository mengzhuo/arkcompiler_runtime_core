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
#ifndef PANDA_RUNTIME_THREAD_H_
#define PANDA_RUNTIME_THREAD_H_

#include <memory>
#include <chrono>
#include <limits>
#include <thread>
#include <atomic>
#include <csignal>

#include "libpandabase/mem/gc_barrier.h"
#include "libpandabase/mem/ringbuf/lock_free_ring_buffer.h"
#include "libpandabase/os/mutex.h"
#include "libpandabase/os/thread.h"
#include "libpandabase/utils/arch.h"
#include "libpandabase/utils/list.h"
#include "libpandabase/utils/tsan_interface.h"
#include "runtime/include/mem/panda_containers.h"
#include "runtime/include/mem/panda_smart_pointers.h"
#include "runtime/include/object_header-inl.h"
#include "runtime/include/stack_walker.h"
#include "runtime/include/language_context.h"
#include "runtime/include/locks.h"
#include "runtime/include/thread_status.h"
#include "runtime/interpreter/cache.h"
#include "runtime/mem/frame_allocator-inl.h"
#include "runtime/mem/gc/gc.h"
#include "runtime/mem/internal_allocator.h"
#include "runtime/mem/tlab.h"
#include "runtime/mem/refstorage/reference_storage.h"
#include "runtime/entrypoints/entrypoints.h"
#include "events/events.h"

#define ASSERT_HAVE_ACCESS_TO_MANAGED_OBJECTS()

namespace panda {

template <class TYPE>
class HandleStorage;
template <class TYPE>
class GlobalHandleStorage;
template <class TYPE>
class HandleScope;

namespace test {
class ThreadTest;
}  // namespace test

class ThreadManager;
class Runtime;
class PandaVM;
class MonitorPool;

namespace mem {
class GCBarrierSet;
}  // namespace mem

namespace tooling {
class PtThreadInfo;
}  // namespace tooling

enum ThreadFlag { NO_FLAGS = 0, SUSPEND_REQUEST = 2, RUNTIME_TERMINATION_REQUEST = 4, SAFEPOINT_REQUEST = 8 };

struct CustomTLSData {
    CustomTLSData() = default;
    virtual ~CustomTLSData() = default;

    NO_COPY_SEMANTIC(CustomTLSData);
    NO_MOVE_SEMANTIC(CustomTLSData);
};

class LockedObjectInfo {
public:
    LockedObjectInfo(ObjectHeader *obj, void *fp) : object_(obj), stack_(fp) {}
    inline ObjectHeader *GetObject() const
    {
        return object_;
    }

    inline void SetObject(ObjectHeader *obj_new)
    {
        object_ = obj_new;
    }

    inline void *GetStack() const
    {
        return stack_;
    }

    inline void SetStack(void *stack_new)
    {
        stack_ = stack_new;
    }

    static constexpr uint32_t GetMonitorOffset()
    {
        return MEMBER_OFFSET(LockedObjectInfo, object_);
    }

    static constexpr uint32_t GetStackOffset()
    {
        return MEMBER_OFFSET(LockedObjectInfo, stack_);
    }

private:
    ObjectHeader *object_;
    void *stack_;
};

template <typename Adapter = mem::AllocatorAdapter<LockedObjectInfo>>
class LockedObjectList {
    static constexpr uint32_t DEFAULT_CAPACITY = 16;

public:
    LockedObjectList() : capacity_(DEFAULT_CAPACITY), allocator_(Adapter())
    {
        storage_ = allocator_.allocate(DEFAULT_CAPACITY);
    }

    ~LockedObjectList()
    {
        allocator_.deallocate(storage_, capacity_);
    }

    NO_COPY_SEMANTIC(LockedObjectList);
    NO_MOVE_SEMANTIC(LockedObjectList);

    void PushBack(LockedObjectInfo data)
    {
        ExtendIfNeeded();
        storage_[size_++] = data;
    }

    template <typename... Args>
    LockedObjectInfo &EmplaceBack(Args &&...args)
    {
        ExtendIfNeeded();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto *raw_mem = &storage_[size_];
        auto *datum = new (raw_mem) LockedObjectInfo(std::forward<Args>(args)...);
        size_++;
        return *datum;
    }

    LockedObjectInfo &Back()
    {
        ASSERT(size_ > 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return storage_[size_ - 1];
    }

    bool Empty() const
    {
        return size_ == 0;
    }

    void PopBack()
    {
        ASSERT(size_ > 0);
        --size_;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        (&storage_[size_])->~LockedObjectInfo();
    }

    Span<LockedObjectInfo> Data()
    {
        return Span<LockedObjectInfo>(storage_, size_);
    }

    static constexpr uint32_t GetCapacityOffset()
    {
        return MEMBER_OFFSET(LockedObjectList, capacity_);
    }

    static constexpr uint32_t GetSizeOffset()
    {
        return MEMBER_OFFSET(LockedObjectList, size_);
    }

    static constexpr uint32_t GetDataOffset()
    {
        return MEMBER_OFFSET(LockedObjectList, storage_);
    }

private:
    void ExtendIfNeeded()
    {
        ASSERT(size_ <= capacity_);
        if (size_ < capacity_) {
            return;
        }
        uint32_t new_capacity = capacity_ * 3U / 2U;  // expand by 1.5
        LockedObjectInfo *new_storage = allocator_.allocate(new_capacity);
        ASSERT(new_storage != nullptr);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::copy(storage_, storage_ + size_, new_storage);
        allocator_.deallocate(storage_, capacity_);
        storage_ = new_storage;
        capacity_ = new_capacity;
    }

    template <typename T, size_t ALIGNMENT = sizeof(T)>
    using Aligned __attribute__((aligned(ALIGNMENT))) = T;
    // Use uint32_t instead of size_t to guarantee the same size
    // on all platforms and simplify compiler stubs accessing this fields.
    // uint32_t is large enough to fit locked objects list's size.
    Aligned<uint32_t> capacity_;
    Aligned<uint32_t> size_ {0};
    Aligned<LockedObjectInfo *> storage_;
    Adapter allocator_;
};

/**
 *  Hierarchy of thread classes
 *
 *         +--------+
 *         | Thread |
 *         +--------+
 *             |
 *      +---------------+
 *      | ManagedThread |
 *      +---------------+
 *             |
 *     +-----------------+
 *     | MTManagedThread |
 *     +-----------------+
 *
 *
 *  Thread - is the most low-level entity. This class contains pointers to VM which this thread associated.
 *  ManagedThread - stores runtime context to run managed code in single-threaded environment
 *  MTManagedThread - extends ManagedThread to be able to run code in multi-threaded environment
 */

/// @brief Class represents arbitrary runtime thread
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class Thread {
public:
    using ThreadId = uint32_t;
    enum class ThreadType {
        THREAD_TYPE_NONE,
        THREAD_TYPE_GC,
        THREAD_TYPE_COMPILER,
        THREAD_TYPE_MANAGED,
        THREAD_TYPE_MT_MANAGED,
        THREAD_TYPE_TASK,
        THREAD_TYPE_WORKER_THREAD,
    };

    Thread(PandaVM *vm, ThreadType thread_type);
    virtual ~Thread();
    NO_COPY_SEMANTIC(Thread);
    NO_MOVE_SEMANTIC(Thread);

    PANDA_PUBLIC_API static Thread *GetCurrent();
    PANDA_PUBLIC_API static void SetCurrent(Thread *thread);

    virtual void FreeInternalMemory();

    void FreeAllocatedMemory();

    PandaVM *GetVM() const
    {
        return vm_;
    }

    void SetVM(PandaVM *vm)
    {
        vm_ = vm;
    }

    MutatorLock *GetMutatorLock()
    {
        return mutator_lock_;
    }

    const MutatorLock *GetMutatorLock() const
    {
        return mutator_lock_;
    }

    void *GetPreWrbEntrypoint() const
    {
        // Atomic with relaxed order reason: only atomicity and modification order consistency needed
        return pre_wrb_entrypoint_.load(std::memory_order_relaxed);
    }

    void SetPreWrbEntrypoint(void *entry)
    {
        pre_wrb_entrypoint_ = entry;
    }

    ThreadType GetThreadType() const
    {
        return thread_type_;
    }

    ALWAYS_INLINE mem::GCBarrierSet *GetBarrierSet() const
    {
        return barrier_set_;
    }

#ifndef NDEBUG
    MutatorLock::MutatorLockState GetLockState() const
    {
        return lock_state_;
    }

    void SetLockState(MutatorLock::MutatorLockState state)
    {
        lock_state_ = state;
    }
#endif

    // pre_buff_ may be destroyed during Detach(), so it should be initialized once more
    void InitPreBuff();

    static constexpr size_t GetVmOffset()
    {
        return MEMBER_OFFSET(Thread, vm_);
    }

private:
    void InitCardTableData(mem::GCBarrierSet *barrier);

protected:
    union __attribute__((__aligned__(4))) FlagsAndThreadStatus {
        FlagsAndThreadStatus() = default;
        ~FlagsAndThreadStatus() = default;
        struct __attribute__((packed)) {
            volatile uint16_t flags;
            volatile enum ThreadStatus status;
        } as_struct;
        volatile uint32_t as_int;
        uint32_t as_nonvolatile_int;
        std::atomic_uint32_t as_atomic;

        NO_COPY_SEMANTIC(FlagsAndThreadStatus);
        NO_MOVE_SEMANTIC(FlagsAndThreadStatus);
    };

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    bool is_compiled_frame_ {false};
    FlagsAndThreadStatus fts_ {};
    ThreadId internal_id_ {0};

    EntrypointsTable entrypoints_ {};
    void *object_ {nullptr};
    Frame *frame_ {nullptr};
    ObjectHeader *exception_ {nullptr};
    uintptr_t native_pc_ {};
    mem::TLAB *tlab_ {nullptr};
    void *card_table_addr_ {nullptr};
    void *card_table_min_addr_ {nullptr};
    std::atomic<void *> pre_wrb_entrypoint_ {nullptr};  // if NOT nullptr, stores pointer to PreWrbFunc and indicates we
                                                        // are currently in concurrent marking phase
    // keeps IRtoC GC PostWrb impl for storing one object
    void *post_wrb_one_object_ {nullptr};
    // keeps IRtoC GC PostWrb impl for storing two objects
    void *post_wrb_two_objects_ {nullptr};
    void *string_class_ptr_ {nullptr};
    PandaVector<ObjectHeader *> *pre_buff_ {nullptr};
    void *language_extension_data_ {nullptr};
#ifndef NDEBUG
    uintptr_t runtime_call_enabled_ {1};
#endif
    PANDA_PUBLIC_API static ThreadFlag initial_thread_flag_;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

private:
    PandaVM *vm_ {nullptr};
    ThreadType thread_type_ {ThreadType::THREAD_TYPE_NONE};
    mem::GCBarrierSet *barrier_set_ {nullptr};
    MutatorLock *mutator_lock_;
#ifndef NDEBUG
    MutatorLock::MutatorLockState lock_state_ = MutatorLock::UNLOCKED;
#endif
#ifndef PANDA_TARGET_WINDOWS
    stack_t signal_stack_ {};
#endif
};

template <typename ThreadT>
class ScopedCurrentThread {
public:
    explicit ScopedCurrentThread(ThreadT *thread) : thread_(thread)
    {
        ASSERT(Thread::GetCurrent() == nullptr);

        // Set current thread
        Thread::SetCurrent(thread_);
    }

    ~ScopedCurrentThread()
    {
        // Reset current thread
        Thread::SetCurrent(nullptr);
    }

    NO_COPY_SEMANTIC(ScopedCurrentThread);
    NO_MOVE_SEMANTIC(ScopedCurrentThread);

private:
    ThreadT *thread_;
};

}  // namespace panda

#ifdef PANDA_TARGET_MOBILE_WITH_NATIVE_LIBS
#include "platforms/mobile/runtime/thread-inl.cpp"
#endif  // PANDA_TARGET_MOBILE_WITH_NATIVE_LIBS

#endif  // PANDA_RUNTIME_THREAD_H_
