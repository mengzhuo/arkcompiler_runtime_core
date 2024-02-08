/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef COMPILER_OPTIMIZER_CODEGEN_CODEGEN_H
#define COMPILER_OPTIMIZER_CODEGEN_CODEGEN_H

/*
Codegen interface for compiler
! Do not use this file in runtime
*/

#include <tuple>
#include "code_info/code_info_builder.h"
#include "compiler_logger.h"
#include "disassembly.h"
#include "frame_info.h"
#include "optimizer/analysis/live_registers.h"
#include "optimizer/code_generator/callconv.h"
#include "optimizer/code_generator/encode.h"
#include "optimizer/code_generator/scoped_tmp_reg.h"
#include "optimizer/code_generator/registers_description.h"
#include "optimizer/code_generator/slow_path.h"
#include "optimizer/code_generator/spill_fill_encoder.h"
#include "optimizer/code_generator/target_info.h"
#include "optimizer/ir/analysis.h"
#include "optimizer/ir/graph.h"
#include "optimizer/ir/graph_visitor.h"
#include "optimizer/optimizations/regalloc/spill_fills_resolver.h"
#include "optimizer/pass_manager.h"
#include "utils/cframe_layout.h"

namespace ark::compiler {
// Maximum size in bytes
constexpr size_t INST_IN_SLOW_PATH = 64;

class Encoder;
class CodeBuilder;
class OsrEntryStub;

inline VRegInfo::Type IrTypeToMetainfoType(DataType::Type type)
{
    switch (type) {
        case DataType::UINT64:
        case DataType::INT64:
            return VRegInfo::Type::INT64;
        case DataType::ANY:
            return VRegInfo::Type::ANY;
        case DataType::UINT32:
        case DataType::UINT16:
        case DataType::UINT8:
        case DataType::INT32:
        case DataType::INT16:
        case DataType::INT8:
            return VRegInfo::Type::INT32;
        case DataType::FLOAT64:
            return VRegInfo::Type::FLOAT64;
        case DataType::FLOAT32:
            return VRegInfo::Type::FLOAT32;
        case DataType::BOOL:
            return VRegInfo::Type::BOOL;
        case DataType::REFERENCE:
            return VRegInfo::Type::OBJECT;
        default:
            UNREACHABLE();
    }
}

class Codegen : public Optimization {
    using EntrypointId = RuntimeInterface::EntrypointId;

public:
    explicit Codegen(Graph *graph);
    NO_MOVE_SEMANTIC(Codegen);
    NO_COPY_SEMANTIC(Codegen);

    ~Codegen() override = default;

    bool RunImpl() override;
    const char *GetPassName() const override;
    bool AbortIfFailed() const override;

    static bool Run(Graph *graph);

    ArenaAllocator *GetAllocator() const
    {
        return allocator_;
    }
    ArenaAllocator *GetLocalAllocator() const
    {
        return localAllocator_;
    }
    FrameInfo *GetFrameInfo() const
    {
        return frameInfo_;
    }
    void SetFrameInfo(FrameInfo *frameInfo)
    {
        frameInfo_ = frameInfo;
    }
    virtual void CreateFrameInfo();

    RuntimeInterface *GetRuntime() const
    {
        return runtime_;
    }
    RegistersDescription *GetRegfile() const
    {
        return regfile_;
    }
    Encoder *GetEncoder() const
    {
        return enc_;
    }
    CallingConvention *GetCallingConvention() const
    {
        return callconv_;
    }

    GraphVisitor *GetGraphVisitor() const
    {
        return visitor_;
    }

    LabelHolder::LabelId GetLabelEntry() const
    {
        return labelEntry_;
    }

    LabelHolder::LabelId GetLabelExit() const
    {
        return labelExit_;
    }

    RuntimeInterface::MethodId GetMethodId()
    {
        return methodId_;
    }

    void SetStartCodeOffset(size_t offset)
    {
        startCodeOffset_ = offset;
    }

    size_t GetStartCodeOffset() const
    {
        return startCodeOffset_;
    }

    void Convert(ArenaVector<Reg> *regsUsage, const ArenaVector<bool> *mask, TypeInfo typeInfo);

    Reg ConvertRegister(Register r, DataType::Type type = DataType::Type::INT64);

    template <size_t SRC_REGS_COUNT>
    constexpr auto ConvertSrcRegisters(Inst *inst)
    {
        auto lastTuple = std::make_tuple(ConvertRegister(inst->GetSrcReg(SRC_REGS_COUNT - 1), inst->GetType()));
        return std::tuple_cat(ConvertSrcRegisters<SRC_REGS_COUNT - 1>(inst), lastTuple);
    }

    template <size_t SRC_REGS_COUNT>
    constexpr auto ConvertRegisters(Inst *inst)
    {
        auto dstTuple = std::make_tuple(ConvertRegister(inst->GetDstReg(), inst->GetType()));
        return std::tuple_cat(dstTuple, ConvertSrcRegisters<SRC_REGS_COUNT>(inst));
    }

    Imm ConvertImmWithExtend(uint64_t imm, DataType::Type type);

    Condition ConvertCc(ConditionCode cc);
    Condition ConvertCcOverflow(ConditionCode cc);

    static inline TypeInfo ConvertDataType(DataType::Type type, Arch arch)
    {
        return TypeInfo::FromDataType(type, arch);
    }

    Arch GetArch() const
    {
        return GetTarget().GetArch();
    }

    Target GetTarget() const
    {
        return target_;
    }

    TypeInfo GetPtrRegType() const
    {
        return target_.GetPtrRegType();
    }

    CodeInfoBuilder *GetCodeBuilder() const
    {
        return codeBuilder_;
    }

    void CreateStackMap(Inst *inst, Inst *user = nullptr);

    void CreateStackMapRec(SaveStateInst *saveState, bool requireVregMap, Inst *targetSite);
    void CreateVRegMap(SaveStateInst *saveState, size_t vregsCount, Inst *targetSite);
    void CreateVreg(const Location &location, Inst *inst, const VirtualRegister &vreg);
    void FillVregIndices(SaveStateInst *saveState);

    void CreateOsrEntry(SaveStateInst *saveState);

    void CreateVRegForRegister(const Location &location, Inst *inst, const VirtualRegister &vreg);

    /// 'live_inputs' shows that inst's source registers should be added the the mask
    template <bool LIVE_INPUTS = false>
    std::pair<RegMask, VRegMask> GetLiveRegisters(Inst *inst);
    // Limits live register set to a number of registers used to pass parameters to the runtime or fastpath call:
    // 1) these ones are saved/restored by caller
    // 2) the remaining ones are saved/restored by the bridge function (aarch only) or by fastpath prologue/epilogue
    void FillOnlyParameters(RegMask *liveRegs, uint32_t numParams, bool isFastpath) const;

    template <typename T, typename... Args>
    T *CreateSlowPath(Inst *inst, Args &&...args);

    void EmitSlowPaths();

    /**
     * Insert tracing code to the generated code. See `Trace` method in the `runtime/entrypoints.cpp`.
     * NOTE(compiler): we should rework parameters assigning algorithm, that is duplicated here.
     * @param params parameters to be passed to the TRACE entrypoint, first parameter must be TraceId value.
     */
    template <typename... Args>
    void InsertTrace(Args &&...params);
    void CallIntrinsic(Inst *inst, RuntimeInterface::IntrinsicId id);

    template <bool IS_FASTPATH, typename... Args>
    void CallEntrypoint(Inst *inst, EntrypointId id, Reg dstReg, RegMask preservedRegs, Args &&...params);

    void CallEntrypointFinalize(RegMask &liveRegs, RegMask &paramsMask, Inst *inst)
    {
        LoadCallerRegisters(liveRegs, VRegMask(), true);

        if (!inst->HasImplicitRuntimeCall()) {
            return;
        }
        for (auto i = 0U; i < paramsMask.size(); i++) {
            if (paramsMask.test(i)) {
                inst->GetSaveState()->GetRootsRegsMask().reset(i);
            }
        }
    }

    // The function is used for calling runtime functions through special bridges.
    // !NOTE Don't use the function for calling runtime without bridges(it save only parameters on stack)
    template <typename... Args>
    void CallRuntime(Inst *inst, EntrypointId id, Reg dstReg, RegMask preservedRegs, Args &&...params);
    template <typename... Args>
    void CallFastPath(Inst *inst, EntrypointId id, Reg dstReg, RegMask preservedRegs, Args &&...params);
    template <typename... Args>
    void CallRuntimeWithMethod(Inst *inst, void *method, EntrypointId eid, Reg dstReg, Args &&...params);
    void SaveRegistersForImplicitRuntime(Inst *inst, RegMask *paramsMask, RegMask *mask);

    void VisitNewArray(Inst *inst);

    void LoadClassFromObject(Reg classReg, Reg objReg);
    void VisitCallIndirect(CallIndirectInst *inst);
    void VisitCall(CallInst *inst);
    void CreateCallIntrinsic(IntrinsicInst *inst);
    void CreateMultiArrayCall(CallInst *callInst);
    void CreateNewObjCall(NewObjectInst *newObj);
    void CreateNewObjCallOld(NewObjectInst *newObj);
    void CreateMonitorCall(MonitorInst *inst);
    void CreateMonitorCallOld(MonitorInst *inst);
    void CreateCheckCastInterfaceCall(Inst *inst);
    void CreateNonDefaultInitClass(ClassInst *initInst);
    void CheckObject(Reg reg, LabelHolder::LabelId label);
    template <bool IS_CLASS = false>
    void CreatePreWRB(Inst *inst, MemRef mem, RegMask preserved = {}, bool storePair = false);
    void CreatePostWRB(Inst *inst, MemRef mem, Reg reg1, Reg reg2 = INVALID_REGISTER);
    void CreatePostWRBForDynamic(Inst *inst, MemRef mem, Reg reg1, Reg reg2);
    void EncodePostWRB(Inst *inst, MemRef mem, Reg reg1, Reg reg2, bool checkObject = true);
    void CreatePostInterRegionBarrier(Inst *inst, MemRef mem, Reg reg1, Reg reg2, bool checkObject);
    void CreatePostInterGenerationalBarrier(Reg base);
    // Creates call to IRtoC PostWrb Entrypoint. Offline means AOT or IRtoC compilation -> type of GC is not known. So
    // Managed Thread keeps pointer to actual IRtoC GC barriers implementation at run-time.
    void CreateOfflineIrtocPostWrb(Inst *inst, MemRef mem, Reg reg1, Reg reg2);
    // Creates call to IRtoC PostWrb Entrypoint. Online means JIT compilation -> we know GC type.
    void CreateOnlineIrtocPostWrbRegionTwoRegs(Inst *inst, MemRef mem, Reg reg1, Reg reg2, bool checkObject);
    void CreateOnlineIrtocPostWrbRegionOneReg(Inst *inst, MemRef mem, Reg reg1, bool checkObject);
    void CreateOnlineIrtocPostWrb(Inst *inst, MemRef mem, Reg reg1, Reg reg2, bool checkObject);
    template <typename... Args>
    void CallBarrier(RegMask liveRegs, VRegMask liveVregs, std::variant<EntrypointId, Reg> entrypoint,
                     Args &&...params);
    void CreateLoadClassFromPLT(Inst *inst, Reg tmpReg, Reg dst, size_t classId);
    void CreateJumpToClassResolverPltShared(Inst *inst, Reg tmpReg, RuntimeInterface::EntrypointId id);
    void CreateLoadTLABInformation(Reg regTlabStart, Reg regTlabSize);
    void CreateCheckForTLABWithConstSize(Inst *inst, Reg regTlabStart, Reg regTlabSize, size_t size,
                                         LabelHolder::LabelId label);
    void CreateDebugRuntimeCallsForNewObject(Inst *inst, Reg regTlabStart, size_t allocSize, RegMask preserved);
    void CreateDebugRuntimeCallsForObjectClone(Inst *inst, Reg dst);
    void CallFastCreateStringFromCharArrayTlab(Inst *inst, Reg dst, Reg offset, Reg count, Reg array,
                                               std::variant<Reg, TypedImm> klass);
    void CreateReturn(const Inst *inst);
    template <typename T>
    void CreateUnaryCheck(Inst *inst, RuntimeInterface::EntrypointId id, DeoptimizeType type, Condition cc);

    // The function alignment up the value from alignment_reg using tmp_reg.
    void CreateAlignmentValue(Reg alignmentReg, Reg tmpReg, size_t alignment);
    void TryInsertImplicitNullCheck(Inst *inst, size_t prevOffset);

    const CFrameLayout &GetFrameLayout() const
    {
        return frameLayout_;
    }

    bool RegisterKeepCallArgument(CallInst *callInst, Reg reg);

    void LoadMethod(Reg dst);
    void LoadFreeSlot(Reg dst);
    void StoreFreeSlot(Reg src);

    ssize_t GetStackOffset(Location location);
    MemRef GetMemRefForSlot(Location location);
    Reg SpReg() const;

    Reg FpReg() const;

    bool HasLiveCallerSavedRegs(Inst *inst);
    void SaveCallerRegisters(RegMask liveRegs, VRegMask liveVregs, bool adjustRegs);
    void LoadCallerRegisters(RegMask liveRegs, VRegMask liveVregs, bool adjustRegs);

    // Initialization internal variables
    void Initialize();
    bool Finalize();
    void IssueDisasm();
    const Disassembly *GetDisasm() const;
    Disassembly *GetDisasm();
    void AddLiveOut(const BasicBlock *bb, const Register reg);
    RegMask GetLiveOut(const BasicBlock *bb) const;

    Reg ThreadReg() const;
    static bool InstEncodedWithLibCall(const Inst *inst, Arch arch);

    void EncodeDynamicCast(Inst *inst, Reg dst, bool dstSigned, Reg src);

    Reg ConvertInstTmpReg(const Inst *inst, DataType::Type type) const;
    Reg ConvertInstTmpReg(const Inst *inst) const;

    bool OffsetFitReferenceTypeSize(uint64_t offset) const;

protected:
    virtual void GeneratePrologue();
    virtual void GenerateEpilogue();

    // Main logic steps
    bool BeginMethod();
    bool VisitGraph();
    void EndMethod();
    bool CopyToCodeCache();
    void DumpCode();

    RegMask GetUsedRegs() const;
    RegMask GetUsedVRegs() const;

    template <typename... Args>
    void FillCallParams(Args &&...params);

    template <size_t IMM_ARRAY_SIZE, typename Arg, typename... Args>
    ALWAYS_INLINE inline void FillCallParamsHandleOperands(
        ParameterInfo *paramInfo, SpillFillInst *regMoves, ArenaVector<Reg> *spMoves,
        [[maybe_unused]] typename std::array<std::pair<Reg, Imm>, IMM_ARRAY_SIZE>::iterator immsIter, Arg &&arg,
        Args &&...params);

    void EmitJump(const BasicBlock *bb);
    bool EmitCallRuntimeCode(Inst *inst, std::variant<EntrypointId, Reg> entrypoint);

    void IntfInlineCachePass(ResolveVirtualInst *resolver, Reg methodReg, Reg tmpReg, Reg objReg);

    template <typename T>
    RuntimeInterface::MethodPtr GetCallerOfUnresolvedMethod(T *resolver);

    void EmitResolveVirtual(ResolveVirtualInst *resolver);
    void EmitResolveUnknownVirtual(ResolveVirtualInst *resolver, Reg methodReg);
    void EmitResolveVirtualAot(ResolveVirtualInst *resolver, Reg methodReg);
    void EmitCallVirtual(CallInst *call);
    void EmitCallResolvedVirtual(CallInst *call);
    void EmitCallStatic(CallInst *call);
    void EmitResolveStatic(ResolveStaticInst *resolver);
    void EmitCallResolvedStatic(CallInst *call);
    void EmitCallDynamic(CallInst *call);
    void FinalizeCall(CallInst *call);

    uint32_t GetVtableShift();
    void CalculateCardIndex(Reg baseReg, ScopedTmpReg *tmp, ScopedTmpReg *tmp1);
    void CreateBuiltinIntrinsic(IntrinsicInst *inst);
    static constexpr int32_t NUM_OF_SRC_BUILTIN = 6;
    static constexpr uint8_t FIRST_OPERAND = 0;
    static constexpr uint8_t SECOND_OPERAND = 1;
    static constexpr uint8_t THIRD_OPERAND = 2;
    static constexpr uint8_t FOURTH_OPERAND = 3;
    static constexpr uint8_t FIFTH_OPERAND = 4;
    using SRCREGS = std::array<Reg, NUM_OF_SRC_BUILTIN>;
    // implementation is generated with compiler/optimizer/templates/intrinsics/intrinsics_codegen.inl.erb
    void FillBuiltin(IntrinsicInst *inst, SRCREGS src, Reg dst);

    template <typename Arg, typename... Args>
    ALWAYS_INLINE inline void AddParamRegsInLiveMasksHandleArgs(ParameterInfo *paramInfo, RegMask *liveRegs,
                                                                VRegMask *liveVregs, Arg param, Args &&...params);

    template <typename... Args>
    void AddParamRegsInLiveMasks(RegMask *liveRegs, VRegMask *liveVregs, Args &&...params);
    template <typename... Args>
    void CreateStubCall(Inst *inst, RuntimeInterface::IntrinsicId intrinsicId, Reg dst, Args &&...params);

    ScopedTmpReg CalculatePreviousTLABAllocSize(Reg reg, LabelHolder::LabelId label);
    friend class IntrinsicCodegenTest;

    virtual void IntrinsicSlowPathEntry(IntrinsicInst *inst);
    virtual void IntrinsicCallRuntimeSaveAll(IntrinsicInst *inst);
    virtual void IntrinsicSaveRegisters(IntrinsicInst *inst);
    virtual void IntrinsicRestoreRegisters(IntrinsicInst *inst);
    virtual void IntrinsicTailCall(IntrinsicInst *inst);

#include "codegen_language_extensions.h"
#include "intrinsics_codegen.inl.h"

private:
    template <typename T>
    void EncodeImms(const T &imms, bool skipFirstLocation);

    static bool EnsureParamsFitIn32Bit(std::initializer_list<std::variant<Reg, TypedImm>> params);

    template <typename... Args>
    void FillPostWrbCallParams(MemRef mem, Args &&...params);

private:
    ArenaAllocator *allocator_;
    ArenaAllocator *localAllocator_;
    // Register description
    RegistersDescription *regfile_;
    // Encoder implementation
    Encoder *enc_;
    // Target architecture calling convention model
    CallingConvention *callconv_;
    // Current execution model implementation
    // Visitor for instructions
    GraphVisitor *visitor_ {};
    CodeInfoBuilder *codeBuilder_ {nullptr};

    ArenaVector<SlowPathBase *> slowPaths_;
    ArenaUnorderedMap<RuntimeInterface::EntrypointId, SlowPathShared *> slowPathsMap_;

    const CFrameLayout frameLayout_;
    FrameInfo *frameInfo_ {nullptr};
    ArenaVector<OsrEntryStub *> osrEntries_;
    RuntimeInterface::MethodId methodId_ {INVALID_ID};
    size_t startCodeOffset_ {0};
    ArenaVector<std::pair<int16_t, int16_t>> vregIndices_;

    RuntimeInterface *runtime_ {nullptr};

    LabelHolder::LabelId labelEntry_ {};
    LabelHolder::LabelId labelExit_ {};

    const Target target_;

    /* Registers that have been allocated by regalloc */
    RegMask usedRegs_ {0};
    RegMask usedVregs_ {0};
    /* Map of BasicBlock to live-out regsiters mask. It is needed in epilogue encoding to avoid overwriting of the
     * live-out registers */
    ArenaUnorderedMap<const BasicBlock *, RegMask> liveOuts_;

    Disassembly disasm_;
    SpillFillsResolver spillFillsResolver_;

    friend class EncodeVisitor;
    friend class BaselineCodegen;
    friend class SlowPathJsCastDoubleToInt32;
};  // Codegen

template <>
constexpr auto Codegen::ConvertSrcRegisters<0>([[maybe_unused]] Inst *inst)
{
    return std::make_tuple();
}

}  // namespace ark::compiler

#include "codegen-inl.h"

#endif  // COMPILER_OPTIMIZER_CODEGEN_CODEGEN_H
