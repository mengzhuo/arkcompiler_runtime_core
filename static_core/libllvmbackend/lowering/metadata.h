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

#ifndef LIBLLVMBACKEND_LOWERING_METADATA_H
#define LIBLLVMBACKEND_LOWERING_METADATA_H

/// @see https://llvm.org/docs/BranchWeightMetadata.html
// NOLINTNEXTLINE(readability-identifier-naming)
namespace panda::llvmbackend::Metadata::BranchWeights {
auto constexpr LIKELY_BRANCH_WEIGHT = 2000;
auto constexpr UNLIKELY_BRANCH_WEIGHT = 1;
}  // namespace panda::llvmbackend::Metadata::BranchWeights

#endif  //  LIBLLVMBACKEND_LOWERING_METADATA_H