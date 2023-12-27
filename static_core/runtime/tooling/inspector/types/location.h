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

#ifndef PANDA_TOOLING_INSPECTOR_TYPES_LOCATION_H
#define PANDA_TOOLING_INSPECTOR_TYPES_LOCATION_H

#include "numeric_id.h"

#include "utils/expected.h"

#include <cstddef>
#include <functional>
#include <string>

namespace panda {
class JsonObject;
class JsonObjectBuilder;
}  // namespace panda

namespace panda::tooling::inspector {
class Location {
public:
    Location(ScriptId scriptId, size_t lineNumber) : scriptId_(scriptId), lineNumber_(lineNumber) {}

    static Expected<Location, std::string> FromJsonProperty(const JsonObject &object, const char *propertyName);

    ScriptId GetScriptId() const
    {
        return scriptId_;
    }

    size_t GetLineNumber() const
    {
        return lineNumber_;
    }

    std::function<void(JsonObjectBuilder &)> ToJson() const;

private:
    ScriptId scriptId_;
    size_t lineNumber_;
};
}  // namespace panda::tooling::inspector

#endif  // PANDA_TOOLING_INSPECTOR_TYPES_LOCATION_H
