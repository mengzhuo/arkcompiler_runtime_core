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

#ifndef COMPILER_OPTIMIZER_OPTIMIZATIONS_REG_ALLOC_GRAPH_COLORING_H
#define COMPILER_OPTIMIZER_OPTIMIZATIONS_REG_ALLOC_GRAPH_COLORING_H

#include "reg_alloc_base.h"
#include "compiler_logger.h"
#include "interference_graph.h"
#include "optimizer/analysis/liveness_analyzer.h"
#include "optimizer/code_generator/registers_description.h"
#include "optimizer/optimizations/regalloc/working_ranges.h"
#include "reg_map.h"
#include "utils/arena_containers.h"
#include "utils/small_vector.h"

namespace panda::compiler {
class RegAllocGraphColoring : public RegAllocBase {
public:
    explicit RegAllocGraphColoring(Graph *graph);
    RegAllocGraphColoring(Graph *graph, size_t regs_count);

    const char *GetPassName() const override
    {
        return "RegAllocGraphColoring";
    }

    bool AbortIfFailed() const override
    {
        return true;
    }

    static const size_t DEFAULT_VECTOR_SIZE = 64;
    using IndexVector = SmallVector<unsigned, DEFAULT_VECTOR_SIZE>;

protected:
    bool Allocate() override;

private:
    void InitWorkingRanges(WorkingRanges *general_ranges, WorkingRanges *fp_ranges);
    void FillPhysicalNodes(InterferenceGraph *ig, WorkingRanges *ranges, ArenaVector<ColorNode *> &physical_nodes);
    void BuildIG(InterferenceGraph *ig, WorkingRanges *ranges, bool remat_constants = false);
    IndexVector PrecolorIG(InterferenceGraph *ig);
    IndexVector PrecolorIG(InterferenceGraph *ig, const RegisterMap &map);
    void BuildBias(InterferenceGraph *ig, const IndexVector &affinity_nodes);
    void AddAffinityEdgesToPhi(InterferenceGraph *ig, const ColorNode &node, IndexVector *affinity_nodes);
    void AddAffinityEdgesToSiblings(InterferenceGraph *ig, const ColorNode &node, IndexVector *affinity_nodes);
    void AddAffinityEdgesToPhysicalNodes(InterferenceGraph *ig, IndexVector *affinity_nodes);
    void AddAffinityEdge(InterferenceGraph *ig, IndexVector *affinity_nodes, const ColorNode &node, LifeIntervals *li);
    bool AllocateRegisters(InterferenceGraph *ig, WorkingRanges *ranges, WorkingRanges *stack_ranges,
                           const RegisterMap &map);
    bool AllocateSlots(InterferenceGraph *ig, WorkingRanges *stack_ranges);
    void Remap(const InterferenceGraph &ig, const RegisterMap &map);
    bool MapSlots(const InterferenceGraph &ig);
    void InitMap(RegisterMap *map, bool is_vector);
    void Presplit(WorkingRanges *ranges);
    void SparseIG(InterferenceGraph *ig, unsigned regs_count, WorkingRanges *ranges, WorkingRanges *stack_ranges);
    void SpillInterval(LifeIntervals *interval, WorkingRanges *ranges, WorkingRanges *stack_ranges);
};
}  // namespace panda::compiler

#endif  // COMPILER_OPTIMIZER_OPTIMIZATIONS_REG_ALLOC_GRAPH_COLORING_H