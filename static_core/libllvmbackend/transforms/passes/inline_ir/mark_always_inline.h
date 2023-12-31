/*
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

#ifndef LIBLLVMBACKEND_TRANSFORMS_PASSES_INLINE_IR_MARK_ALWAYS_INLINE_H
#define LIBLLVMBACKEND_TRANSFORMS_PASSES_INLINE_IR_MARK_ALWAYS_INLINE_H

#include <llvm/IR/PassManager.h>

namespace panda::llvmbackend {
struct LLVMCompilerOptions;
}  // namespace panda::llvmbackend

namespace panda::llvmbackend::passes {

/**
 * Set AlwaysInline attribute to calls in root functions
 *
 * Root function is a function without FUNCTION_MD_INLINE_MODULE metadata
 */
class MarkAlwaysInline : public llvm::PassInfoMixin<MarkAlwaysInline> {
public:
    static bool ShouldInsert(const panda::llvmbackend::LLVMCompilerOptions *options);

    // NOLINTNEXTLINE(readability-identifier-naming)
    llvm::PreservedAnalyses run(llvm::Function &function, llvm::FunctionAnalysisManager &analysis_manager);

private:
    bool InlineCallTree(llvm::Function *function, int32_t level, int32_t max_level);

public:
    static constexpr llvm::StringRef ARG_NAME = "mark-always-inline";
};

}  // namespace panda::llvmbackend::passes

#endif  // LIBLLVMBACKEND_TRANSFORMS_PASSES_INLINE_IR_MARK_ALWAYS_INLINE_H