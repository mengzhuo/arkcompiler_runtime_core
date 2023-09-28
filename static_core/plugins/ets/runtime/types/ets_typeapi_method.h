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

#ifndef PANDA_PLUGINS_ETS_TYPEAPI_METHOD_H_
#define PANDA_PLUGINS_ETS_TYPEAPI_METHOD_H_

#include "plugins/ets/runtime/types/ets_object.h"
#include "plugins/ets/runtime/types/ets_string.h"
#include "types/ets_primitives.h"
#include "types/ets_typeapi_feature.h"

namespace panda::ets {

class EtsCoroutine;

class EtsTypeAPIMethod : public ObjectHeader {
public:
    EtsTypeAPIMethod() = delete;
    ~EtsTypeAPIMethod() = delete;

    NO_COPY_SEMANTIC(EtsTypeAPIMethod);
    NO_MOVE_SEMANTIC(EtsTypeAPIMethod);

    static EtsTypeAPIMethod *Create(EtsCoroutine *ets_coroutine = EtsCoroutine::GetCurrent());

    EtsObject *AsObject()
    {
        return EtsObject::FromCoreType(this);
    }

    const EtsObject *AsObject() const
    {
        return EtsObject::FromCoreType(this);
    }

    static EtsTypeAPIMethod *FromEtsObject(EtsObject *field)
    {
        return reinterpret_cast<EtsTypeAPIMethod *>(field);
    }

    void SetTypeDesc(const char *td)
    {
        auto coro = EtsCoroutine::GetCurrent();
        [[maybe_unused]] HandleScope<ObjectHeader *> scope {coro};
        VMHandle<EtsTypeAPIMethod> self {coro, this};
        // allocate beforehand to ensure order and keep self/this pointer valid in SetObject
        auto val = EtsString::CreateFromMUtf8(td)->AsObject()->GetCoreType();
        ObjectAccessor::SetObject(self.GetPtr(), MEMBER_OFFSET(EtsTypeAPIMethod, td_), val);
    }

    void SetName(EtsString *name)
    {
        ObjectAccessor::SetObject(this, MEMBER_OFFSET(EtsTypeAPIMethod, name_), name->AsObject()->GetCoreType());
    }

    void SetAccessMod(EtsTypeAPIFeatureAccessModifier access_mod)
    {
        ObjectAccessor::SetPrimitive(this, MEMBER_OFFSET(EtsTypeAPIMethod, access_mod_), access_mod);
    }

    void SetAttributes(EtsInt attr)
    {
        ObjectAccessor::SetPrimitive(this, MEMBER_OFFSET(EtsTypeAPIMethod, attr_), attr);
    }

private:
    ObjectPointer<EtsString> td_;
    ObjectPointer<EtsString> name_;
    FIELD_UNUSED EtsInt attr_;  // note alignment
    FIELD_UNUSED EtsByte access_mod_;
};

}  // namespace panda::ets

#endif  // PANDA_PLUGINS_ETS_TYPEAPI_METHOD_H
