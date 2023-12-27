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

#include <cstddef>
#include <cstdint>

#ifndef PANDA_VERIFIER_UTIL_STRUCT_FIELD_H__
#define PANDA_VERIFIER_UTIL_STRUCT_FIELD_H__

namespace panda::verifier {
template <typename S, typename T>
struct StructField {
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    size_t offset;
    explicit StructField(size_t pOffst) : offset {pOffst} {}
    T &Of(S &s) const
    {
        return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(&s) + offset);
    }
};
}  // namespace panda::verifier

#endif  // !PANDA_VERIFIER_UTIL_STRUCT_FIELD_H__
