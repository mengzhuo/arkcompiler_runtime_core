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

#ifndef COMPILER_OPTIMIZER_PASS_MANAGER_H
#define COMPILER_OPTIMIZER_PASS_MANAGER_H

#include <tuple>
#include "compiler_options.h"
#include "pass.h"
#include "utils/bit_field.h"
#include "utils/arena_containers.h"
#include "pass_manager_statistics.h"

namespace panda::compiler {
class Graph;
class Pass;
class Analysis;
class CatchInputs;
class LivenessAnalyzer;
class LiveRegisters;
class LoopAnalyzer;
class AliasAnalysis;
class DominatorsTree;
class Rpo;
class LinearOrder;
class BoundsAnalysis;
class MonitorAnalysis;
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ObjectTypePropagation;
class RegAllocVerifier;
class TypesAnalysis;

namespace details {
template <typename... Types>
class PassTypeList {
public:
    using IdentifierType = size_t;
    using TupleType = std::tuple<std::decay_t<Types>...>;

    template <typename T>
    static constexpr bool HasType()
    {
        return std::disjunction_v<std::is_same<T, Types>...>;
    }

    template <typename T, typename... Args>
    static ArenaVector<T> Instantiate(ArenaAllocator *allocator, Args &&...args)
    {
        ArenaVector<T> vec(allocator->Adapter());
        vec.reserve(sizeof...(Types));
        ((vec.push_back(allocator->New<Types>((std::forward<Args>(args))...))), ...);
        return vec;
    }

    template <typename Type, std::size_t... INDEXES>
    static constexpr size_t GetIndex(std::index_sequence<INDEXES... /* unused */>)
    {
        static_assert(HasType<Type>());
        return (0 + ... +
                (std::is_same_v<Type, std::tuple_element_t<INDEXES, TupleType>> ? size_t(INDEXES) : size_t {}));
    }

    template <typename Type>
    static constexpr IdentifierType ID = GetIndex<std::decay_t<Type>>(std::index_sequence_for<Types...> {});
    static constexpr size_t SIZE = sizeof...(Types);
};

using PredefinedAnalyses =
    PassTypeList<LivenessAnalyzer, LoopAnalyzer, AliasAnalysis, DominatorsTree, Rpo, LinearOrder, BoundsAnalysis,
                 MonitorAnalysis, LiveRegisters, ObjectTypePropagation, RegAllocVerifier, TypesAnalysis, CatchInputs>;
}  // namespace details

class PassManager {
public:
    PassManager(Graph *graph, PassManager *parent_pm);

    ArenaAllocator *GetAllocator();
    ArenaAllocator *GetLocalAllocator();

    bool RunPass(Pass *pass, size_t local_mem_size_before_pass);

    template <typename T, typename... Args>
    bool RunPass(Args... args)
    {
        auto local_mem_size_before = GetLocalAllocator()->GetAllocatedSize();
        bool res = false;
        // NOLINTNEXTLINE(readability-braces-around-statements)
        if constexpr (details::PredefinedAnalyses::HasType<T>()) {
            static_assert(sizeof...(Args) == 0);
            res = RunPass(analyses_[details::PredefinedAnalyses::ID<T>], local_mem_size_before);
            // NOLINTNEXTLINE(readability-misleading-indentation)
        } else {
            T pass(graph_, std::forward<Args>(args)...);
            res = RunPass(&pass, local_mem_size_before);
        }
        if (!IsCheckMode()) {
            if (OPTIONS.IsCompilerResetLocalAllocator()) {
                ASSERT(GetLocalAllocator() != GetAllocator());
                GetLocalAllocator()->Resize(local_mem_size_before);
            }
        }
        return res;
    }

    template <typename T>
    T &GetAnalysis()
    {
        static_assert(std::is_base_of_v<Analysis, std::decay_t<T>>);
        return *static_cast<T *>(analyses_[details::PredefinedAnalyses::ID<T>]);
    }
    std::string GetFileName(const char *pass_name = nullptr, const std::string &suffix = ".cfg");
    void DumpGraph(const char *pass_name);
    void DumpLifeIntervals(const char *pass_name);
    void InitialDumpVisualizerGraph();
    void DumpVisualizerGraph(const char *pass_name);

    Graph *GetGraph()
    {
        return graph_;
    }

    void Finalize() const;

    PassManagerStatistics *GetStatistics()
    {
        return stats_;
    }

    void SetCheckMode(bool v)
    {
        check_mode_ = v;
    }

    bool IsCheckMode() const
    {
        return check_mode_;
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    size_t GetExecutionCounter() const
    {
        return execution_counter_;
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void StartExecution()
    {
        execution_counter_++;
    }

private:
    Graph *graph_ {nullptr};
    ArenaVector<Optimization *> optimizations_;
    const ArenaVector<Analysis *> analyses_;

    PassManagerStatistics *stats_ {nullptr};
    inline static size_t execution_counter_ {0};

    // Whether passes are run by checker.
    bool check_mode_ {false};

    bool first_execution_ {true};
};
}  // namespace panda::compiler

#endif  // COMPILER_OPTIMIZER_PASS_MANAGER_H
