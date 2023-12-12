/**
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "tests/unit_test.h"
#include "optimizer/ir/graph_cloner.h"
#include "optimizer/optimizations/deoptimize_elimination.h"
#include "optimizer/optimizations/cleanup.h"
#include "plugins/ets/compiler/optimizer/optimizations/interop_js/interop_intrinsic_optimization.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTRINSIC(ID, INTRINSIC_ID) \
    INST(ID, Opcode::Intrinsic).IntrinsicId(RuntimeInterface::IntrinsicId::INTRINSIC_##INTRINSIC_ID)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PARAM_TEST(CLASS_NAME, TEST_NAME, ...)                    \
    class TEST_NAME : public CLASS_NAME {};                       \
    INSTANTIATE_TEST_SUITE_P(CLASS_NAME, TEST_NAME, __VA_ARGS__); \
    TEST_P(TEST_NAME, test)

namespace panda::compiler {

enum BlockPos : uint8_t { PRED = 1U << 0U, LEFT = 1U << 1U, RIGHT = 1U << 2U, NEXT = 1U << 3U, NONE = 0U };

BlockPos operator|(BlockPos lhs, BlockPos rhs)
{
    return static_cast<BlockPos>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class InteropIntrnsicsOptTest : public GraphTest, public testing::WithParamInterface<uint8_t> {
public:
    bool BlockEnabled(BlockPos block_pos)
    {
        return (GetParam() & block_pos) != 0;
    }
};

std::string GetTestName(const testing::TestParamInfo<InteropIntrnsicsOptTest::ParamType> &info)
{
    static std::array<std::string_view, 4U> block_pos_names {"Pred", "Left", "Right", "Next"};
    auto param = info.param;
    if (param == BlockPos::NONE) {
        return "None";
    }
    std::stringstream res;
    for (size_t i = 0; i < block_pos_names.size(); ++i) {
        if ((param & (1U << i)) != 0U) {
            res << block_pos_names[i];
        }
    }
    return res.str();
}

// NOLINTBEGIN(readability-magic-numbers)

// Check that two adjacent scopes are merged and
TEST_F(InteropIntrnsicsOptTest, SingleBlock)
{
    GRAPH(GetGraph())
    {
        PARAMETER(0U, 0U).s32();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(2U, 2U).ptr();
        CONSTANT(3U, 2U).s64();

        BASIC_BLOCK(2U, -1L)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            INTRINSIC(6U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(0U, 4U);
            INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, 6U, 6U, 4U);
            INTRINSIC(8U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(7U, 4U);
            INST(9U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(10U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(9U);

            INST(11U, Opcode::Mul).s32().Inputs(0U, 0U);

            INST(12U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(13U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(12U);
            INTRINSIC(14U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(11U, 12U);
            INTRINSIC(15U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 12U);
            INTRINSIC(16U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, 14U, 15U, 12U);
            INTRINSIC(17U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);

            INST(18U, Opcode::ReturnVoid).v0id();
        }
    }

    Graph *graph = CreateEmptyGraph();
    GRAPH(graph)
    {
        PARAMETER(0U, 0U).s32();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(2U, 2U).ptr();
        CONSTANT(3U, 2U).s64();

        BASIC_BLOCK(2U, -1L)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            INTRINSIC(6U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(0U, 4U);
            INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, 6U, 6U, 4U);

            INST(11U, Opcode::Mul).s32().Inputs(0U, 0U);

            INST(12U, Opcode::SaveState).NoVregs();
            INTRINSIC(14U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(11U, 12U);
            INTRINSIC(16U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, 14U, 7U, 12U);
            INTRINSIC(17U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);

            INST(18U, Opcode::ReturnVoid).v0id();
        }
    }

    ASSERT_TRUE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
    GetGraph()->Dump(&std::cerr);
    // DeoptimizeElimination removes SaveState user of ConvertLocalToJSValue
    ASSERT_TRUE(GetGraph()->RunPass<DeoptimizeElimination>());
    GetGraph()->Dump(&std::cerr);
    ASSERT_TRUE(GetGraph()->RunPass<Cleanup>());
    GetGraph()->Dump(&std::cerr);
    GraphChecker(GetGraph()).Check();
    ASSERT_TRUE(GraphComparator().Compare(GetGraph(), graph));
}

TEST_F(InteropIntrnsicsOptTest, SingleBlockNotApplied)
{
    GRAPH(GetGraph())
    {
        PARAMETER(0U, 0U).s32();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(2U, 2U).ptr();
        CONSTANT(3U, 2U).s64();

        BASIC_BLOCK(2U, -1L)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            INTRINSIC(6U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(0U, 4U);
            INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, 6U, 6U, 4U);
            INTRINSIC(8U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(7U, 4U);
            INST(9U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(10U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(9U);

            INST(19U, Opcode::SaveState).NoVregs();
            INST(20U, Opcode::CallStatic).v0id().InputsAutoType(19U);

            INST(12U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(13U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(12U);
            INTRINSIC(14U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(0U, 12U);
            INTRINSIC(15U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 12U);
            INTRINSIC(16U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, 14U, 15U, 12U);
            INTRINSIC(17U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);

            INST(18U, Opcode::ReturnVoid).v0id();
        }
    }

    auto initial = GraphCloner(GetGraph(), GetGraph()->GetAllocator(), GetGraph()->GetLocalAllocator()).CloneGraph();
    ASSERT_FALSE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
    ASSERT_TRUE(GraphComparator().Compare(GetGraph(), initial));
}

PARAM_TEST(InteropIntrnsicsOptTest, ChainOfBlocks,
           ::testing::Values(BlockPos::PRED, BlockPos::LEFT, BlockPos::NEXT, BlockPos::NONE), GetTestName)
{
    GRAPH(GetGraph())
    {
        PARAMETER(0U, 0U).s32();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(2U, 2U).ptr();
        CONSTANT(3U, 2U).s64();

        BASIC_BLOCK(2U, 3U)
        {
            INST(18U, Opcode::Add).s32().Inputs(0U, 0U);
        }
        BASIC_BLOCK(3U, 4U)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            INTRINSIC(6U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(0U, 4U);
            INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, 6U, 6U, 4U);
            INTRINSIC(8U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(7U, 4U);
            INST(9U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(10U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(9U);

            if (BlockEnabled(BlockPos::PRED)) {
                INST(23U, Opcode::SaveState).NoVregs();
                INST(24U, Opcode::CallStatic).v0id().InputsAutoType(23U);
            }
            INST(19U, Opcode::Add).s32().Inputs(18U, 0U);
        }
        BASIC_BLOCK(4U, 5U)
        {
            if (BlockEnabled(BlockPos::LEFT)) {
                INST(23U, Opcode::SaveState).NoVregs();
                INST(24U, Opcode::CallStatic).v0id().InputsAutoType(23U);
            }
            INST(20U, Opcode::Add).s32().Inputs(19U, 0U);
        }
        BASIC_BLOCK(5U, -1L)
        {
            INST(21U, Opcode::Add).s32().Inputs(20U, 0U);
            if (BlockEnabled(BlockPos::NEXT)) {
                INST(23U, Opcode::SaveState).NoVregs();
                INST(24U, Opcode::CallStatic).v0id().InputsAutoType(23U);
            }

            INST(12U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(13U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(12U);
            INTRINSIC(14U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(21U, 12U);
            INTRINSIC(15U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 12U);
            INTRINSIC(16U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, 14U, 15U, 12U);
            INTRINSIC(17U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);

            INST(22U, Opcode::ReturnVoid).v0id();
        }
    }
    GraphChecker(GetGraph()).Check();
    if (GetParam() == BlockPos::NONE) {
        Graph *graph = CreateEmptyGraph();
        GRAPH(graph)
        {
            PARAMETER(0U, 0U).s32();
            PARAMETER(1U, 1U).ptr();
            PARAMETER(2U, 2U).ptr();
            CONSTANT(3U, 2U).s64();

            BASIC_BLOCK(2U, -1L)
            {
                INST(18U, Opcode::Add).s32().Inputs(0U, 0U);
                INST(4U, Opcode::SaveState).NoVregs();
                INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
                INTRINSIC(6U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(0U, 4U);
                INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, 6U, 6U, 4U);

                INST(19U, Opcode::Add).s32().Inputs(18U, 0U);
                INST(20U, Opcode::Add).s32().Inputs(19U, 0U);
                INST(21U, Opcode::Add).s32().Inputs(20U, 0U);

                INST(12U, Opcode::SaveState).NoVregs();
                INTRINSIC(14U, COMPILER_CONVERT_I32_TO_LOCAL).ptr().InputsAutoType(21U, 12U);
                INTRINSIC(16U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, 14U, 7U, 12U);
                INTRINSIC(17U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);

                INST(22U, Opcode::ReturnVoid).v0id();
            }
        }
        ASSERT_TRUE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
        // DeoptimizeElimination removes SaveState user of ConvertLocalToJSValue
        ASSERT_TRUE(GetGraph()->RunPass<DeoptimizeElimination>());
        ASSERT_TRUE(GetGraph()->RunPass<Cleanup>());
        GraphChecker(GetGraph()).Check();
        ASSERT_TRUE(GraphComparator().Compare(GetGraph(), graph));
    } else {
        ASSERT(INS(24U).GetOpcode() == Opcode::CallStatic);
        auto initial =
            GraphCloner(GetGraph(), GetGraph()->GetAllocator(), GetGraph()->GetLocalAllocator()).CloneGraph();
        ASSERT_FALSE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
        ASSERT_TRUE(GraphComparator().Compare(GetGraph(), initial));
    }
}

// Param specifies for which blocks there is a local scope
PARAM_TEST(InteropIntrnsicsOptTest, TriangleOfBlocks,
           ::testing::Values(BlockPos::PRED | BlockPos::LEFT | BlockPos::NEXT, BlockPos::PRED | BlockPos::LEFT,
                             BlockPos::PRED | BlockPos::NEXT, BlockPos::LEFT | BlockPos::NEXT),
           GetTestName)
{
    GRAPH(GetGraph())
    {
        PARAMETER(0U, 0U).s32();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(2U, 2U).ptr();
        CONSTANT(3U, 1U).s64();
        CONSTANT(23U, 0U).s64();

        BASIC_BLOCK(2U, 3U, 4U)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            if (BlockEnabled(BlockPos::PRED)) {
                INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
                INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 23U, 4U);
                INTRINSIC(8U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(7U, 4U);
                INST(9U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
                INTRINSIC(10U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(9U);
            } else {
                INTRINSIC(8U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(4U);
            }
            INST(11U, Opcode::IfImm).CC(compiler::CC_EQ).Imm(0U).Inputs(0U);
        }
        BASIC_BLOCK(3U, 4U)
        {
            if (BlockEnabled(BlockPos::LEFT)) {
                INST(24U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
                INTRINSIC(25U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(24U);
                INTRINSIC(27U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 24U);
                INTRINSIC(28U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, 27U, 24U);
                INTRINSIC(29U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(24U);
            }
        }
        BASIC_BLOCK(4U, -1L)
        {
            INST(30U, Opcode::Phi).s64().Inputs(3U, 23U);
            if (BlockEnabled(BlockPos::NEXT)) {
                INST(12U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
                INTRINSIC(13U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(12U);
                INTRINSIC(15U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 12U);
                INTRINSIC(16U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, 15U, 12U);
                INTRINSIC(17U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);
            }

            INST(22U, Opcode::Return).s64().Inputs(30U);
        }
    }
    GraphChecker(GetGraph()).Check();
    if (BlockEnabled(BlockPos::PRED) && BlockEnabled(BlockPos::NEXT)) {
        Graph *graph = CreateEmptyGraph();
        GRAPH(graph)
        {
            PARAMETER(0U, 0U).s32();
            PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
            PARAMETER(2U, 2U).ptr();
            CONSTANT(3U, 1U).s64();
            CONSTANT(23U, 0U).s64();

            BASIC_BLOCK(2U, 3U, 4U)
            {
                INST(4U, Opcode::SaveState).NoVregs();
                INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
                INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 23U, 4U);
                INST(11U, Opcode::IfImm).CC(compiler::CC_EQ).Imm(0U).Inputs(0U);
            }
            BASIC_BLOCK(3U, 4U)
            {
                if (BlockEnabled(BlockPos::LEFT)) {
                    INST(24U, Opcode::SaveState).NoVregs();
                    INTRINSIC(28U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, 7U, 24U);
                }
            }
            BASIC_BLOCK(4U, -1L)
            {
                INST(30U, Opcode::Phi).s64().Inputs(3U, 23U);
                INST(12U, Opcode::SaveState).NoVregs();
                INTRINSIC(16U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, 7U, 12U);
                INTRINSIC(17U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);

                INST(22U, Opcode::Return).s64().Inputs(30U);
            }
        }
        ASSERT_TRUE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
        ASSERT_TRUE(GetGraph()->RunPass<DeoptimizeElimination>());
        ASSERT_TRUE(GetGraph()->RunPass<Cleanup>());
        GraphChecker(GetGraph()).Check();
        ASSERT_TRUE(GraphComparator().Compare(GetGraph(), graph));
    } else {
        auto initial =
            GraphCloner(GetGraph(), GetGraph()->GetAllocator(), GetGraph()->GetLocalAllocator()).CloneGraph();
        ASSERT_FALSE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
        ASSERT_TRUE(GraphComparator().Compare(GetGraph(), initial));
    }
}

// Param specifies for which blocks there is a local scope
PARAM_TEST(InteropIntrnsicsOptTest, DiamondOfBlocks,
           ::testing::Values(BlockPos::PRED | BlockPos::RIGHT | BlockPos::NEXT, BlockPos::PRED | BlockPos::RIGHT,
                             BlockPos::PRED | BlockPos::NEXT, BlockPos::RIGHT | BlockPos::NEXT),
           GetTestName)
{
    GRAPH(GetGraph())
    {
        PARAMETER(0U, 0U).s32();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(2U, 2U).ptr();
        PARAMETER(32U, 3U).ptr();
        CONSTANT(23U, 0U).s64();
        CONSTANT(3U, 1U).s64();
        CONSTANT(42U, 2U).s64();

        BASIC_BLOCK(2U, 3U, 4U)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            if (BlockEnabled(BlockPos::PRED)) {
                INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
                INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 23U, 4U);
                INTRINSIC(8U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(7U, 4U);
                INST(9U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
                INTRINSIC(10U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(9U);
            } else {
                INTRINSIC(8U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(4U);
            }
            INST(11U, Opcode::IfImm).CC(compiler::CC_EQ).Imm(0U).Inputs(0U);
        }
        BASIC_BLOCK(3U, 5U)
        {
            INST(24U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(25U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(24U);
            INTRINSIC(27U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 24U);
            INTRINSIC(28U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 32U, 3U, 27U, 24U);
            INTRINSIC(29U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(28U, 24U);
            INST(30U, Opcode::SaveState).Inputs(8U, 29U).SrcVregs({0U, 1U});
            INTRINSIC(31U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(30U);
        }
        BASIC_BLOCK(4U, 5U)
        {
            INST(33U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            if (BlockEnabled(BlockPos::RIGHT)) {
                INTRINSIC(34U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(33U);
                INTRINSIC(35U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 33U);
                INTRINSIC(36U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, 35U, 33U);
                INTRINSIC(37U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(36U, 33U);
                INST(38U, Opcode::SaveState).Inputs(8U, 37U).SrcVregs({0U, 1U});
                INTRINSIC(39U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(38U);
            } else {
                INTRINSIC(37U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(33U);
            }
        }
        BASIC_BLOCK(5U, -1L)
        {
            INST(40U, Opcode::Phi).ref().Inputs(29U, 37U);
            if (BlockEnabled(BlockPos::NEXT)) {
                INST(12U, Opcode::SaveState).Inputs(8U, 40U).SrcVregs({0U, 2U});
                INTRINSIC(13U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(12U);
                INTRINSIC(14U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 12U);
                INTRINSIC(15U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(40U, 12U);
                INTRINSIC(16U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 42U, 14U, 15U, 12U);
                INTRINSIC(17U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);
                INST(41U, Opcode::ReturnVoid).v0id();
            } else {
                INST(41U, Opcode::Return).ref().Inputs(40U);
            }
        }
    }
    Graph *graph = CreateEmptyGraph();
    int js_val {};
    GRAPH(graph)
    {
        PARAMETER(0U, 0U).s32();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(2U, 2U).ptr();
        PARAMETER(32U, 3U).ptr();
        CONSTANT(23U, 0U).s64();
        CONSTANT(3U, 1U).s64();
        CONSTANT(42U, 2U).s64();

        BASIC_BLOCK(2U, 3U, 4U)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            if (BlockEnabled(BlockPos::PRED)) {
                INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 23U, 4U);
                js_val = 7;
            } else {
                INTRINSIC(8U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(4U);
            }
            INST(11U, Opcode::IfImm).CC(compiler::CC_EQ).Imm(0U).Inputs(0U);
        }
        BASIC_BLOCK(3U, 5U)
        {
            INST(24U, Opcode::SaveState).Inputs(8U).SrcVregs({0U}).CleanupInputs();
            if (!BlockEnabled(BlockPos::PRED)) {
                INTRINSIC(27U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 24U);
                js_val = 27;
            }
            INTRINSIC(28U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 32U, 3U, js_val, 24U);
            INTRINSIC(29U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(28U, 24U);
            if (!BlockEnabled(BlockPos::NEXT)) {
                INST(30U, Opcode::SaveState).Inputs(8U, 29U).SrcVregs({0U, 1U}).CleanupInputs();
                INTRINSIC(31U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(30U);
            }
        }
        BASIC_BLOCK(4U, 5U)
        {
            INST(33U, Opcode::SaveState).Inputs(8U).SrcVregs({0U}).CleanupInputs();
            if (BlockEnabled(BlockPos::RIGHT)) {
                if (!BlockEnabled(BlockPos::PRED)) {
                    INTRINSIC(35U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 33U);
                    js_val = 35;
                }
                INTRINSIC(36U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, js_val, 33U);
                INTRINSIC(37U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(36U, 33U);
                if (!BlockEnabled(BlockPos::NEXT)) {
                    INST(38U, Opcode::SaveState).Inputs(8U, 37U).SrcVregs({0U, 1U}).CleanupInputs();
                    INTRINSIC(39U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(38U);
                }
            } else {
                INTRINSIC(37U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(33U);
            }
        }
        BASIC_BLOCK(5U, -1L)
        {
            INST(40U, Opcode::Phi).ref().Inputs(29U, 37U);
            if (BlockEnabled(BlockPos::NEXT)) {
                INST(12U, Opcode::SaveState).Inputs(8U, 40U).SrcVregs({0U, 2U}).CleanupInputs();
                if (!BlockEnabled(BlockPos::PRED)) {
                    INTRINSIC(14U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(8U, 12U);
                    js_val = 14;
                }
                // NOTE(aefremov): unwrap of Phi inputs and wrap of Phi may be removed
                // in a way similar to PhiTypeResolving pass
                INTRINSIC(15U, COMPILER_CONVERT_JS_VALUE_TO_LOCAL).ptr().InputsAutoType(40U, 12U);
                INTRINSIC(16U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 42U, js_val, 15U, 12U);
                INTRINSIC(17U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);
                INST(41U, Opcode::ReturnVoid).v0id();
            } else {
                INST(41U, Opcode::Return).ref().Inputs(40U);
            }
        }
    }
    ASSERT_TRUE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
    ASSERT_TRUE(GetGraph()->RunPass<DeoptimizeElimination>());
    ASSERT_TRUE(GetGraph()->RunPass<Cleanup>());
    // Remove unused constants for some parameter values
    graph->RunPass<Cleanup>();
    GraphChecker(GetGraph()).Check();
    ASSERT_TRUE(GraphComparator().Compare(GetGraph(), graph));
}

// Replace a wrap in dominated block with a wrap in dominator
// In some cases hoist the wrap to the dominator
// Param specifies for which blocks there is a wrap of int value v0 to js value
PARAM_TEST(InteropIntrnsicsOptTest, DiamondOfBlocksRepeatedWraps,
           ::testing::Values(BlockPos::PRED | BlockPos::LEFT | BlockPos::RIGHT | BlockPos::NEXT,
                             BlockPos::LEFT | BlockPos::RIGHT, BlockPos::LEFT | BlockPos::RIGHT | BlockPos::NEXT,
                             BlockPos::LEFT | BlockPos::NEXT, BlockPos::PRED | BlockPos::NEXT, BlockPos::NEXT),
           GetTestName)
{
    int js_val {};
    GRAPH(GetGraph())
    {
        PARAMETER(0U, 0U).s32();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(2U, 2U).ptr();
        PARAMETER(32U, 3U).ptr();
        CONSTANT(3U, 1U).s64();

        BASIC_BLOCK(2U, 3U, 4U)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            if (BlockEnabled(BlockPos::PRED)) {
                INTRINSIC(8U, COMPILER_CONVERT_I32_TO_LOCAL).ref().InputsAutoType(0U, 4U);
                js_val = 8;
            } else {
                INTRINSIC(9U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(4U);
                js_val = 9;
            }
            INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, js_val, 4U);
            INTRINSIC(10U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            INST(11U, Opcode::IfImm).CC(compiler::CC_EQ).Imm(0U).Inputs(0U);
        }
        BASIC_BLOCK(3U, 5U)
        {
            INST(24U, Opcode::SaveState).NoVregs();
            INTRINSIC(25U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(24U);
            if (BlockEnabled(BlockPos::LEFT)) {
                INTRINSIC(27U, COMPILER_CONVERT_I32_TO_LOCAL).ref().InputsAutoType(0U, 24U);
            } else {
                INTRINSIC(27U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(24U);
            }
            INTRINSIC(28U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 32U, 3U, 27U, 24U);
            INTRINSIC(29U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(24U);
        }
        BASIC_BLOCK(4U, 5U)
        {
            INST(33U, Opcode::SaveState).NoVregs();
            INTRINSIC(34U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(33U);
            if (BlockEnabled(BlockPos::RIGHT)) {
                INTRINSIC(35U, COMPILER_CONVERT_I32_TO_LOCAL).ref().InputsAutoType(0U, 33U);
            } else {
                INTRINSIC(35U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(33U);
            }
            INTRINSIC(36U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, 35U, 33U);
            INTRINSIC(37U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(33U);
        }
        BASIC_BLOCK(5U, -1L)
        {
            INST(12U, Opcode::SaveState).NoVregs();
            INTRINSIC(13U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(12U);
            if (BlockEnabled(BlockPos::NEXT)) {
                INTRINSIC(14U, COMPILER_CONVERT_I32_TO_LOCAL).ref().InputsAutoType(0U, 12U);
            } else {
                INTRINSIC(14U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(12U);
            }
            INTRINSIC(15U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(1U, 2U, 3U, 14U, 12U);
            INTRINSIC(16U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);
            INST(17U, Opcode::ReturnVoid).v0id();
        }
    }

    Graph *graph = CreateEmptyGraph();
    bool wrap_in_pred = GetParam() != (BlockPos::LEFT | BlockPos::RIGHT) && GetParam() != BlockPos::NEXT;
    GRAPH(graph)
    {
        PARAMETER(0U, 0U).s32();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(2U, 2U).ptr();
        PARAMETER(32U, 3U).ptr();
        CONSTANT(3U, 1U).s64();

        BASIC_BLOCK(2U, 3U, 4U)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            if (wrap_in_pred) {
                INTRINSIC(8U, COMPILER_CONVERT_I32_TO_LOCAL).ref().InputsAutoType(0U, 4U);
                js_val = 8;
            }
            if (!BlockEnabled(BlockPos::PRED)) {
                INTRINSIC(9U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(4U);
                js_val = 9;
            }
            INTRINSIC(7U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, js_val, 4U);
            INST(11U, Opcode::IfImm).CC(compiler::CC_EQ).Imm(0U).Inputs(0U);
        }
        BASIC_BLOCK(3U, 5U)
        {
            INST(24U, Opcode::SaveState).Inputs(8U).SrcVregs({VirtualRegister::BRIDGE}).CleanupInputs();
            if (!BlockEnabled(BlockPos::LEFT)) {
                INTRINSIC(27U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(24U);
                js_val = 27;
            } else if (wrap_in_pred) {
                js_val = 8;
            } else {
                INTRINSIC(27U, COMPILER_CONVERT_I32_TO_LOCAL).ref().InputsAutoType(0U, 24U);
                js_val = 27;
            }
            INTRINSIC(28U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 32U, 3U, js_val, 24U);
        }
        BASIC_BLOCK(4U, 5U)
        {
            INST(33U, Opcode::SaveState).Inputs(8U).SrcVregs({VirtualRegister::BRIDGE}).CleanupInputs();
            if (!BlockEnabled(BlockPos::RIGHT)) {
                INTRINSIC(35U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(33U);
                js_val = 35;
            } else if (wrap_in_pred) {
                js_val = 8;
            } else {
                INTRINSIC(35U, COMPILER_CONVERT_I32_TO_LOCAL).ref().InputsAutoType(0U, 33U);
                js_val = 35;
            }
            INTRINSIC(36U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, js_val, 33U);
        }
        BASIC_BLOCK(5U, -1L)
        {
            if (BlockEnabled(BlockPos::NEXT) && wrap_in_pred) {
                INST(12U, Opcode::SaveState).Inputs(8U).SrcVregs({VirtualRegister::BRIDGE});
                js_val = 8;
            } else {
                INST(12U, Opcode::SaveState).NoVregs();
                if (BlockEnabled(BlockPos::NEXT)) {
                    INTRINSIC(14U, COMPILER_CONVERT_I32_TO_LOCAL).ref().InputsAutoType(0U, 12U);
                } else {
                    INTRINSIC(14U, JS_RUNTIME_GET_UNDEFINED).ref().InputsAutoType(12U);
                }
                js_val = 14;
            }
            INTRINSIC(15U, COMPILER_JS_CALL_VOID_FUNCTION).v0id().InputsAutoType(1U, 2U, 3U, js_val, 12U);
            INTRINSIC(16U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(12U);
            INST(17U, Opcode::ReturnVoid).v0id();
        }
    }
    ASSERT_TRUE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
    GetGraph()->RunPass<Cleanup>();

    GraphChecker(GetGraph()).Check();
    ASSERT_TRUE(GraphComparator().Compare(GetGraph(), graph));
}

TEST_F(InteropIntrnsicsOptTest, WrapUnwrapReturnValue)
{
    GRAPH(GetGraph())
    {
        PARAMETER(0U, 0U).ptr();
        PARAMETER(1U, 1U).ptr();  // Pass this and function values as parameters to simplify the test
        CONSTANT(3U, 0U).s64();

        BASIC_BLOCK(2U, -1L)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(0U, 1U, 3U, 4U);
            INTRINSIC(8U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(7U, 4U);
            INST(9U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(10U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(9U);

            INTRINSIC(11U, JS_RUNTIME_GET_VALUE_DOUBLE).f64().InputsAutoType(8U, 9U);
            INST(18U, Opcode::Return).f64().Inputs(11U);
        }
    }

    Graph *graph = CreateEmptyGraph();
    GRAPH(graph)
    {
        PARAMETER(0U, 0U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(1U, 1U).ptr();
        CONSTANT(3U, 0U).s64();

        BASIC_BLOCK(2U, -1L)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(0U, 1U, 3U, 4U);
            INTRINSIC(8U, COMPILER_CONVERT_LOCAL_TO_F64).f64().InputsAutoType(7U, 4U);
            INST(9U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(10U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(9U);

            INST(18U, Opcode::Return).f64().Inputs(8U);
        }
    }

    ASSERT_TRUE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
    GraphChecker(GetGraph()).Check();
    ASSERT_TRUE(GraphComparator().Compare(GetGraph(), graph));
}

// ConvertLocalToJSValue has another user (return), so we do not merge it with GetValueDouble
TEST_F(InteropIntrnsicsOptTest, WrapUnwrapReturnValueNotApplied)
{
    GRAPH(GetGraph())
    {
        PARAMETER(0U, 0U).ptr();  // Pass this and function values as parameters to simplify the test
        PARAMETER(1U, 1U).ptr();
        CONSTANT(3U, 0U).s64();

        BASIC_BLOCK(2U, -1L)
        {
            INST(4U, Opcode::SaveState).NoVregs();
            INTRINSIC(5U, COMPILER_CREATE_LOCAL_SCOPE).v0id().InputsAutoType(4U);
            INTRINSIC(7U, COMPILER_JS_CALL_FUNCTION).ptr().InputsAutoType(0U, 1U, 3U, 4U);
            INTRINSIC(8U, COMPILER_CONVERT_LOCAL_TO_JS_VALUE).ref().InputsAutoType(7U, 4U);
            INST(9U, Opcode::SaveState).Inputs(8U).SrcVregs({0U});
            INTRINSIC(10U, COMPILER_DESTROY_LOCAL_SCOPE).v0id().InputsAutoType(9U);

            INTRINSIC(11U, JS_RUNTIME_GET_VALUE_DOUBLE).f64().InputsAutoType(8U, 9U);
            INST(12U, Opcode::CallStatic).v0id().InputsAutoType(11U, 9U);
            INST(18U, Opcode::Return).ref().Inputs(8U);
        }
    }

    auto initial = GraphCloner(GetGraph(), GetGraph()->GetAllocator(), GetGraph()->GetLocalAllocator()).CloneGraph();
    ASSERT_FALSE(GetGraph()->RunPass<InteropIntrinsicOptimization>());
    ASSERT_TRUE(GraphComparator().Compare(GetGraph(), initial));
}

// NOLINTEND(readability-magic-numbers)

}  // namespace panda::compiler