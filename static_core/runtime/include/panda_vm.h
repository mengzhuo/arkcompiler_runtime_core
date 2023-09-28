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
#ifndef PANDA_RUNTIME_PANDA_VM_H_
#define PANDA_RUNTIME_PANDA_VM_H_

#include "include/coretypes/string.h"
#include "include/runtime_options.h"
#include "runtime/include/locks.h"
#include "runtime/include/mem/panda_containers.h"
#include "runtime/include/mem/panda_string.h"
#include "runtime/include/method.h"
#include "runtime/include/runtime.h"
#include "runtime/mem/gc/gc_phase.h"

#include "libpandabase/utils/expected.h"

namespace panda {

class ManagedThread;
class StringTable;
class ThreadManager;

namespace compiler {
class RuntimeInterface;
}  // namespace compiler

namespace mem {
class HeapManager;
class GC;
class GCTrigger;
class ReferenceProcessor;
}  // namespace mem

enum class PandaVMType : size_t { ECMA_VM };  // Deprecated. Only for Compability with js_runtime.

class PandaVM {
public:
    static PandaVM *Create(Runtime *runtime, const RuntimeOptions &options, std::string_view runtime_type);

    explicit PandaVM() : mutator_lock_(Locks::NewMutatorLock()) {};

    virtual ~PandaVM() = default;

    static PandaVM *GetCurrent()
    {
        ASSERT(Thread::GetCurrent() != nullptr);
        return Thread::GetCurrent()->GetVM();
    }

    virtual coretypes::String *ResolveString([[maybe_unused]] const panda_file::File &pf,
                                             [[maybe_unused]] panda_file::File::EntityId id)
    {
        return nullptr;
    }
    virtual bool Initialize() = 0;
    virtual bool InitializeFinish() = 0;
    virtual void PreStartup() = 0;
    virtual void PreZygoteFork() = 0;
    virtual void PostZygoteFork() = 0;
    virtual void InitializeGC() = 0;
    virtual void StartGC() = 0;
    virtual void StopGC() = 0;
    virtual void VisitVmRoots(const GCRootVisitor &visitor);
    virtual void UpdateVmRefs();
    virtual void UninitializeThreads() = 0;
    virtual void SaveProfileInfo() {}

    virtual Expected<int, Runtime::Error> InvokeEntrypoint(Method *entrypoint, const std::vector<std::string> &args);

    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    virtual void HandleReferences([[maybe_unused]] const GCTask &task,
                                  [[maybe_unused]] const mem::GC::ReferenceClearPredicateT &pred)
    {
    }
    virtual void HandleEnqueueReferences() {}
    virtual void HandleGCFinished() {}
    /*
     * Handle some vm related routine (e.g. finalization calls) after GC.
     * Method will be called only in a mutator thread
     */
    virtual void HandleGCRoutineInMutator() {}
    virtual void HandleLdaStr(Frame *frame, BytecodeId string_id);
    virtual void HandleReturnFrame() {}

    virtual mem::GCStats *GetGCStats() const = 0;
    virtual mem::HeapManager *GetHeapManager() const = 0;
    virtual mem::GC *GetGC() const = 0;
    virtual mem::GCTrigger *GetGCTrigger() const = 0;
    virtual const RuntimeOptions &GetOptions() const = 0;
    virtual ManagedThread *GetAssociatedThread() const = 0;
    virtual StringTable *GetStringTable() const = 0;
    virtual mem::MemStatsType *GetMemStats() const = 0;
    virtual Rendezvous *GetRendezvous() const = 0;
    virtual mem::GlobalObjectStorage *GetGlobalObjectStorage() const = 0;
    virtual MonitorPool *GetMonitorPool() const = 0;
    virtual ThreadManager *GetThreadManager() const = 0;
    virtual coretypes::String *CreateString([[maybe_unused]] Method *ctor, [[maybe_unused]] ObjectHeader *obj)
    {
        UNREACHABLE();
        return nullptr;
    }
    virtual compiler::RuntimeInterface *GetCompilerRuntimeInterface() const
    {
        return nullptr;
    }
    virtual void VisitStringTable(const StringTable::StringVisitor &visitor, mem::VisitGCRootFlags flags)
    {
        GetStringTable()->VisitRoots(visitor, flags);
    }

    virtual void VisitStrings(const StringTable::StringVisitor &visitor)
    {
        GetStringTable()->VisitStrings(visitor);
    }

    virtual void SweepVmRefs(const GCObjectVisitor &gc_object_visitor)
    {
        GetStringTable()->Sweep(gc_object_visitor);
    }

    virtual bool UpdateMovedStrings()
    {
        return GetStringTable()->UpdateMoved();
    }

    // TODO(maksenov): remove this method after fixing interpreter performance
    virtual PandaVMType GetPandaVMType() const
    {
        // Deprecated. Only for Compability with js_runtime.
        return PandaVMType::ECMA_VM;
    }

    virtual LanguageContext GetLanguageContext() const = 0;
    virtual CompilerInterface *GetCompiler() const = 0;

    virtual panda::mem::ReferenceProcessor *GetReferenceProcessor() const = 0;

    virtual ObjectHeader *GetOOMErrorObject() = 0;

    virtual void RegisterSignalHandlers() {};

    virtual void DumpForSigQuit([[maybe_unused]] std::ostream &os) const {};

    virtual std::unique_ptr<const panda_file::File> OpenPandaFile(std::string_view location);

    virtual coretypes::String *ResolveStringFromCompiledCode(const panda_file::File &pf, panda_file::File::EntityId id)
    {
        return ResolveString(pf, id);
    }

    virtual coretypes::String *ResolveString([[maybe_unused]] Frame *frame,
                                             [[maybe_unused]] panda_file::File::EntityId id)
    {
        return nullptr;
    }

    virtual coretypes::String *GetNonMovableString(const panda_file::File &pf, panda_file::File::EntityId id) const;

    uint32_t GetFrameExtSize() const
    {
        return frame_ext_size_;
    }

    virtual bool IsBytecodeProfilingEnabled() const
    {
        return false;
    }

    virtual void CleanUpTask([[maybe_unused]] Method *method) {};

    virtual bool ShouldEnableDebug();

    virtual bool IsStaticProfileEnabled() const
    {
        return false;
    }

    virtual void DumpHeap(PandaOStringStream *o_str) const
    {
        size_t obj_cnt = 0;
        *o_str << "Dumping heap" << std::endl;
        GetHeapManager()->GetObjectAllocator()->IterateOverObjects([&obj_cnt, &o_str](ObjectHeader *mem) {
            DumpObject(mem, o_str);
            obj_cnt++;
        });
        *o_str << "Total dumped " << obj_cnt << std::endl;
    }

    virtual PandaString GetClassesFootprint() const;

    void LoadDebuggerAgent()
    {
        debugger_agent_ = CreateDebuggerAgent();
    }

    void UnloadDebuggerAgent()
    {
        debugger_agent_.reset();
    }

    MutatorLock *GetMutatorLock()
    {
        return mutator_lock_;
    }

    const MutatorLock *GetMutatorLock() const
    {
        return mutator_lock_;
    }

    // Intrusive GC test API
    void MarkObject(ObjectHeader *obj);
    void IterateOverMarkQueue(const std::function<void(ObjectHeader *)> &visitor);
    void ClearMarkQueue();

    virtual void ClearInteropHandleScopes([[maybe_unused]] Frame *frame) {}

    NO_MOVE_SEMANTIC(PandaVM);
    NO_COPY_SEMANTIC(PandaVM);

protected:
    virtual bool CheckEntrypointSignature(Method *entrypoint) = 0;
    virtual Expected<int, Runtime::Error> InvokeEntrypointImpl(Method *entrypoint,
                                                               const std::vector<std::string> &args) = 0;
    virtual void HandleUncaughtException() = 0;

    void SetFrameExtSize(uint32_t size)
    {
        frame_ext_size_ = size;
    }

    virtual LoadableAgentHandle CreateDebuggerAgent();

private:
    /// Lock used for preventing object heap modifications (for example at GC<->JIT,ManagedCode interaction during STW)
    MutatorLock *mutator_lock_;
    uint32_t frame_ext_size_ {EMPTY_EXT_FRAME_DATA_SIZE};
    LoadableAgentHandle debugger_agent_;

    // Intrusive GC test API
    PandaList<ObjectHeader *> mark_queue_ GUARDED_BY(mark_queue_lock_);
    os::memory::Mutex mark_queue_lock_;
};

}  // namespace panda

#endif  // PANDA_RUNTIME_PANDA_VM_H_
