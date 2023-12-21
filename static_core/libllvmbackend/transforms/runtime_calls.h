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

#ifndef LIBLLVMBACKEND_TRANSFORMS_RUNTIME_CALLS_H
#define LIBLLVMBACKEND_TRANSFORMS_RUNTIME_CALLS_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

namespace panda::llvmbackend {

class LLVMArkInterface;

}  // namespace panda::llvmbackend

namespace panda::llvmbackend::runtime_calls {

llvm::Value *GetAddressToTLS(llvm::IRBuilder<> *builder, LLVMArkInterface *ark_interface, uintptr_t tls_offset);
llvm::Value *LoadTLSValue(llvm::IRBuilder<> *builder, LLVMArkInterface *ark_interface, uintptr_t tls_offset,
                          llvm::Type *type);

using EntrypointId = int;
llvm::CallInst *CreateEntrypointCallCommon(llvm::IRBuilder<> *builder, llvm::Value *thread_reg_value,
                                           LLVMArkInterface *ark_interface, EntrypointId eid,
                                           llvm::ArrayRef<llvm::Value *> arguments = llvm::None);

llvm::Value *GetThreadRegValue(llvm::IRBuilder<> *builder, LLVMArkInterface *ark_interface);
llvm::Value *GetRealFrameRegValue(llvm::IRBuilder<> *builder, LLVMArkInterface *ark_interface);

}  // namespace panda::llvmbackend::runtime_calls

#endif  // LIBLLVMBACKEND_TRANSFORMS_RUNTIME_CALLS_H