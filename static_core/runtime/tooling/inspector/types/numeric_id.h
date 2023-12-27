/**
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef PANDA_TOOLING_INSPECTOR_TYPES_NUMERIC_ID_H
#define PANDA_TOOLING_INSPECTOR_TYPES_NUMERIC_ID_H

#include "utils/expected.h"
#include "utils/json_parser.h"
#include "utils/string_helpers.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

namespace panda::tooling::inspector {
using BreakpointId = size_t;
using FrameId = uint32_t;
using RemoteObjectId = size_t;
using ScriptId = size_t;

template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
Expected<T, std::string> ParseNumericId(const JsonObject &object, const char *propertyName)
{
    auto property = object.GetValue<JsonObject::StringT>(propertyName);
    if (!property) {
        return Unexpected(std::string("No such property: ") + propertyName);
    }

    intmax_t value;
    if (!helpers::string::ParseInt(*property, &value, INTMAX_C(0)) || value < 0 ||
        std::numeric_limits<T>::max() < static_cast<uintmax_t>(value)) {
        return Unexpected("Invalid id: " + *property);
    }

    return static_cast<T>(value);
}
}  // namespace panda::tooling::inspector

#endif  // PANDA_TOOLING_INSPECTOR_TYPES_NUMERIC_ID_H
