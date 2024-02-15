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

#ifndef COMPILER_OPTIMIZER_OPTIMIZATIONS_INLINING_H
#define COMPILER_OPTIMIZER_OPTIMIZATIONS_INLINING_H

#include <string>
#include "optimizer/pass.h"
#include "optimizer/ir/inst.h"
#include "optimizer/ir/runtime_interface.h"
#include "compiler_options.h"
#include "utils/arena_containers.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage,-warnings-as-errors)
#define LOG_INLINING(level) COMPILER_LOG(level, INLINING) << GetLogIndent()

namespace ark::compiler {
struct InlineContext {
    RuntimeInterface::MethodPtr method {};
    bool chaDevirtualize {false};
    bool replaceToStatic {false};
    RuntimeInterface::IntrinsicId intrinsicId {RuntimeInterface::IntrinsicId::INVALID};
};

struct InlinedGraph {
    Graph *graph {nullptr};
    bool hasRuntimeCalls {false};
};

class Inlining : public Optimization {
    static constexpr size_t SMALL_METHOD_MAX_SIZE = 5;

public:
    explicit Inlining(Graph *graph) : Inlining(graph, 0, 0, 0) {}
    Inlining(Graph *graph, bool resolveWoInline) : Inlining(graph)
    {
        resolveWoInline_ = resolveWoInline;
    }

    Inlining(Graph *graph, uint32_t instructionsCount, uint32_t depth, uint32_t methodsInlined);

    NO_MOVE_SEMANTIC(Inlining);
    NO_COPY_SEMANTIC(Inlining);
    ~Inlining() override = default;

    bool RunImpl() override;

    bool IsEnable() const override
    {
        return g_options.IsCompilerInlining();
    }

    const char *GetPassName() const override
    {
        return "Inline";
    }

    void InvalidateAnalyses() override;

protected:
    virtual void RunOptimizations() const;
    virtual bool IsInstSuitableForInline(Inst *inst) const;
    virtual bool TryInline(CallInst *callInst);
    bool TryInlineWithInlineCaches(CallInst *callInst);
    bool TryInlineExternal(CallInst *callInst, InlineContext *ctx);
    bool TryInlineExternalAot(CallInst *callInst, InlineContext *ctx);

    Inst *GetNewDefAndCorrectDF(Inst *callInst, Inst *oldDef);

    bool Do();
    bool DoInline(CallInst *callInst, InlineContext *ctx);
    bool DoInlineMethod(CallInst *callInst, InlineContext *ctx);
    bool DoInlineIntrinsic(CallInst *callInst, InlineContext *ctx);
    bool DoInlineMonomorphic(CallInst *callInst, RuntimeInterface::ClassPtr receiver);
    bool DoInlinePolymorphic(CallInst *callInst, ArenaVector<RuntimeInterface::ClassPtr> *receivers);
    void CreateCompareClass(CallInst *callInst, Inst *getClsInst, RuntimeInterface::ClassPtr receiver,
                            BasicBlock *callBb);
    void InsertDeoptimizeInst(CallInst *callInst, BasicBlock *callBb,
                              DeoptimizeType deoptType = DeoptimizeType::INLINE_IC);
    void InsertCallInst(CallInst *callInst, BasicBlock *callBb, BasicBlock *retBb, Inst *phiInst);

    void UpdateDataflow(Graph *graphInl, Inst *callInst, std::variant<BasicBlock *, PhiInst *> use,
                        Inst *newDef = nullptr);
    void UpdateDataflowForEmptyGraph(Inst *callInst, std::variant<BasicBlock *, PhiInst *> use, BasicBlock *endBlock);
    void UpdateParameterDataflow(Graph *graphInl, Inst *callInst);
    void UpdateControlflow(Graph *graphInl, BasicBlock *callBb, BasicBlock *callContBb);
    void MoveConstants(Graph *graphInl);

    template <bool CHECK_EXTERNAL, bool CHECK_INTRINSICS = false>
    bool CheckMethodCanBeInlined(const CallInst *callInst, InlineContext *ctx);
    template <bool CHECK_EXTERNAL>
    bool CheckTooBigMethodCanBeInlined(const CallInst *callInst, InlineContext *ctx, bool methodIsTooBig);
    bool ResolveTarget(CallInst *callInst, InlineContext *ctx);
    bool CanUseTypeInfo(ObjectTypeInfo typeInfo, RuntimeInterface::MethodPtr method);
    void InsertChaGuard(CallInst *callInst);

    InlinedGraph BuildGraph(InlineContext *ctx, CallInst *callInst, CallInst *polyCallInst = nullptr);
    bool CheckBytecode(CallInst *callInst, const InlineContext &ctx, bool *calleeCallRuntime);
    bool CheckInstructionLimit(CallInst *callInst, InlineContext *ctx, size_t inlinedInstsCount);
    bool TryBuildGraph(const InlineContext &ctx, Graph *graphInl, CallInst *callInst, CallInst *polyCallInst);
    bool CheckLoops(bool *calleeCallRuntime, Graph *graphInl);
    static void PropagateObjectInfo(Graph *graphInl, CallInst *callInst);

    void ProcessCallReturnInstructions(CallInst *callInst, BasicBlock *callContBb, bool hasRuntimeCalls,
                                       bool needBarriers = false);
    size_t CalculateInstructionsCount(Graph *graph);

    IClassHierarchyAnalysis *GetCha()
    {
        return cha_;
    }

    bool IsInlineCachesEnabled() const;

    std::string GetLogIndent() const
    {
        return std::string(depth_ * 2U, ' ');
    }

    bool IsIntrinsic(const InlineContext *ctx) const
    {
        return ctx->intrinsicId != RuntimeInterface::IntrinsicId::INVALID;
    }

    virtual bool SkipBlock(const BasicBlock *block) const;

protected:
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    uint32_t depth_ {0};
    uint32_t methodsInlined_ {0};
    uint32_t instructionsCount_ {0};
    uint32_t instructionsLimit_ {0};
    ArenaVector<BasicBlock *> returnBlocks_;
    ArenaUnorderedSet<std::string> blacklist_;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

private:
    uint32_t vregsCount_ {0};
    bool resolveWoInline_ {false};
    IClassHierarchyAnalysis *cha_ {nullptr};
};
}  // namespace ark::compiler

#endif  // COMPILER_OPTIMIZER_OPTIMIZATIONS_INLINING_H
