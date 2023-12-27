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

#include "ets_coroutine.h"
#include "ets_vm.h"
#include "intrinsics.h"
#include "mem/vm_handle.h"
#include "plugins/ets/runtime/types/ets_object.h"
#include "plugins/ets/runtime/types/ets_string.h"
#include "plugins/ets/runtime/types/ets_box_primitive-inl.h"
#include "plugins/ets/runtime/types/ets_void.h"
#include "plugins/ets/runtime/ets_panda_file_items.h"
#include "types/ets_array.h"
#include "types/ets_box_primitive.h"
#include "types/ets_class.h"
#include "types/ets_method.h"
#include "types/ets_primitives.h"
#include "types/ets_type.h"
#include "types/ets_type_comptime_traits.h"
#include "types/ets_typeapi.h"
#include "types/ets_typeapi_field.h"

namespace panda::ets::intrinsics {

EtsVoid *ValueAPISetFieldObject(EtsObject *obj, EtsLong i, EtsObject *val)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObject> objHandle(coroutine, obj->GetCoreType());
    VMHandle<EtsObject> valHandle(coroutine, val->GetCoreType());

    auto typeClass = objHandle.GetPtr()->GetClass();
    auto fieldObject = typeClass->GetFieldByIndex(i);
    objHandle.GetPtr()->SetFieldObject(fieldObject, valHandle.GetPtr());
    return EtsVoid::GetInstance();
}

template <typename T>
void SetFieldValue(EtsObject *obj, EtsLong i, T val)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObject> objHandle(coroutine, obj->GetCoreType());

    auto typeClass = objHandle.GetPtr()->GetClass();
    auto fieldObject = typeClass->GetFieldByIndex(i);
    if (fieldObject->GetType()->IsBoxedClass()) {
        objHandle.GetPtr()->SetFieldObject(fieldObject, EtsBoxPrimitive<T>::Create(coroutine, val));
        return;
    }
    objHandle.GetPtr()->SetFieldPrimitive<T>(fieldObject, val);
}

EtsVoid *ValueAPISetFieldBoolean(EtsObject *obj, EtsLong i, EtsBoolean val)
{
    SetFieldValue(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldByte(EtsObject *obj, EtsLong i, EtsByte val)
{
    SetFieldValue(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldShort(EtsObject *obj, EtsLong i, EtsShort val)
{
    SetFieldValue(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldChar(EtsObject *obj, EtsLong i, EtsChar val)
{
    SetFieldValue(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldInt(EtsObject *obj, EtsLong i, EtsInt val)
{
    SetFieldValue(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldLong(EtsObject *obj, EtsLong i, EtsLong val)
{
    SetFieldValue(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldFloat(EtsObject *obj, EtsLong i, EtsFloat val)
{
    SetFieldValue(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldDouble(EtsObject *obj, EtsLong i, EtsDouble val)
{
    SetFieldValue(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldByNameObject(EtsObject *obj, EtsString *name, EtsObject *val)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObject> objHandle(coroutine, obj->GetCoreType());
    VMHandle<EtsString> nameHandle(coroutine, name->GetCoreType());
    VMHandle<EtsObject> valHandle(coroutine, val->GetCoreType());

    auto typeClass = objHandle.GetPtr()->GetClass();
    auto fieldObject = typeClass->GetFieldIDByName(nameHandle.GetPtr()->GetMutf8().c_str());
    objHandle.GetPtr()->SetFieldObject(fieldObject, valHandle.GetPtr());
    return EtsVoid::GetInstance();
}

template <typename T>
void SetFieldByNameValue(EtsObject *obj, EtsString *name, T val)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObject> objHandle(coroutine, obj->GetCoreType());
    VMHandle<EtsString> nameHandle(coroutine, name->GetCoreType());

    auto typeClass = objHandle.GetPtr()->GetClass();
    auto fieldObject = typeClass->GetFieldIDByName(nameHandle.GetPtr()->GetMutf8().c_str());
    if (fieldObject->GetType()->IsBoxedClass()) {
        objHandle.GetPtr()->SetFieldObject(fieldObject, EtsBoxPrimitive<T>::Create(coroutine, val));
        return;
    }
    objHandle.GetPtr()->SetFieldPrimitive<T>(fieldObject, val);
}

EtsVoid *ValueAPISetFieldByNameBoolean(EtsObject *obj, EtsString *name, EtsBoolean val)
{
    SetFieldByNameValue(obj, name, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldByNameByte(EtsObject *obj, EtsString *name, EtsByte val)
{
    SetFieldByNameValue(obj, name, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldByNameShort(EtsObject *obj, EtsString *name, EtsShort val)
{
    SetFieldByNameValue(obj, name, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldByNameChar(EtsObject *obj, EtsString *name, EtsChar val)
{
    SetFieldByNameValue(obj, name, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldByNameInt(EtsObject *obj, EtsString *name, EtsInt val)
{
    SetFieldByNameValue(obj, name, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldByNameLong(EtsObject *obj, EtsString *name, EtsLong val)
{
    SetFieldByNameValue(obj, name, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldByNameFloat(EtsObject *obj, EtsString *name, EtsFloat val)
{
    SetFieldByNameValue(obj, name, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetFieldByNameDouble(EtsObject *obj, EtsString *name, EtsDouble val)
{
    SetFieldByNameValue(obj, name, val);
    return EtsVoid::GetInstance();
}

EtsObject *ValueAPIGetFieldObject(EtsObject *obj, EtsLong i)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObject> objHandle(coroutine, obj->GetCoreType());

    auto typeClass = objHandle.GetPtr()->GetClass();
    auto fieldObject = typeClass->GetFieldByIndex(i);
    return objHandle.GetPtr()->GetFieldObject(fieldObject);
}

template <typename T>
T GetFieldValue(EtsObject *obj, EtsLong i)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObject> objHandle(coroutine, obj->GetCoreType());

    auto typeClass = objHandle.GetPtr()->GetClass();
    auto fieldObject = typeClass->GetFieldByIndex(i);

    if (fieldObject->GetType()->IsBoxedClass()) {
        return EtsBoxPrimitive<T>::FromCoreType(objHandle.GetPtr()->GetFieldObject(fieldObject))->GetValue();
    }
    return objHandle.GetPtr()->GetFieldPrimitive<T>(fieldObject);
}

EtsBoolean ValueAPIGetFieldBoolean(EtsObject *obj, EtsLong i)
{
    return GetFieldValue<EtsBoolean>(obj, i);
}

EtsByte ValueAPIGetFieldByte(EtsObject *obj, EtsLong i)
{
    return GetFieldValue<EtsByte>(obj, i);
}

EtsShort ValueAPIGetFieldShort(EtsObject *obj, EtsLong i)
{
    return GetFieldValue<EtsShort>(obj, i);
}

EtsChar ValueAPIGetFieldChar(EtsObject *obj, EtsLong i)
{
    return GetFieldValue<EtsChar>(obj, i);
}

EtsInt ValueAPIGetFieldInt(EtsObject *obj, EtsLong i)
{
    return GetFieldValue<EtsInt>(obj, i);
}

EtsLong ValueAPIGetFieldLong(EtsObject *obj, EtsLong i)
{
    return GetFieldValue<EtsLong>(obj, i);
}

EtsFloat ValueAPIGetFieldFloat(EtsObject *obj, EtsLong i)
{
    return GetFieldValue<EtsFloat>(obj, i);
}

EtsDouble ValueAPIGetFieldDouble(EtsObject *obj, EtsLong i)
{
    return GetFieldValue<EtsDouble>(obj, i);
}

EtsObject *ValueAPIGetFieldByNameObject(EtsObject *obj, EtsString *name)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObject> objHandle(coroutine, obj->GetCoreType());

    auto typeClass = objHandle.GetPtr()->GetClass();
    auto fieldObject = typeClass->GetFieldIDByName(name->GetMutf8().c_str());
    return objHandle.GetPtr()->GetFieldObject(fieldObject);
}

template <typename T>
T GetFieldByNameValue(EtsObject *obj, EtsString *name)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObject> objHandle(coroutine, obj->GetCoreType());

    auto typeClass = objHandle.GetPtr()->GetClass();
    auto fieldObject = typeClass->GetFieldIDByName(name->GetMutf8().c_str());
    if (fieldObject->GetType()->IsBoxedClass()) {
        return EtsBoxPrimitive<T>::FromCoreType(objHandle.GetPtr()->GetFieldObject(fieldObject))->GetValue();
    }
    return objHandle.GetPtr()->GetFieldPrimitive<T>(fieldObject);
}

EtsBoolean ValueAPIGetFieldByNameBoolean(EtsObject *obj, EtsString *name)
{
    return GetFieldByNameValue<EtsBoolean>(obj, name);
}

EtsByte ValueAPIGetFieldByNameByte(EtsObject *obj, EtsString *name)
{
    return GetFieldByNameValue<EtsByte>(obj, name);
}

EtsShort ValueAPIGetFieldByNameShort(EtsObject *obj, EtsString *name)
{
    return GetFieldByNameValue<EtsShort>(obj, name);
}

EtsChar ValueAPIGetFieldByNameChar(EtsObject *obj, EtsString *name)
{
    return GetFieldByNameValue<EtsChar>(obj, name);
}

EtsInt ValueAPIGetFieldByNameInt(EtsObject *obj, EtsString *name)
{
    return GetFieldByNameValue<EtsInt>(obj, name);
}

EtsLong ValueAPIGetFieldByNameLong(EtsObject *obj, EtsString *name)
{
    return GetFieldByNameValue<EtsLong>(obj, name);
}

EtsFloat ValueAPIGetFieldByNameFloat(EtsObject *obj, EtsString *name)
{
    return GetFieldByNameValue<EtsFloat>(obj, name);
}

EtsDouble ValueAPIGetFieldByNameDouble(EtsObject *obj, EtsString *name)
{
    return GetFieldByNameValue<EtsDouble>(obj, name);
}

EtsLong ValueAPIGetArrayLength(EtsObject *obj)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsArray> arrHandle(coroutine, obj->GetCoreType());
    return arrHandle->GetLength();
}

EtsVoid *ValueAPISetElementObject(EtsObject *obj, EtsLong i, EtsObject *val)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObjectArray> arrHandle(coroutine, obj->GetCoreType());
    VMHandle<EtsObject> valHandle(coroutine, val->GetCoreType());

    arrHandle.GetPtr()->Set(i, valHandle.GetPtr());
    return EtsVoid::GetInstance();
}

template <typename P, typename T>
void SetElement(EtsObject *obj, EtsLong i, T val)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    auto typeClass = obj->GetClass();
    if (!typeClass->GetComponentType()->IsBoxedClass()) {
        VMHandle<P> arrHandle(coroutine, obj->GetCoreType());
        arrHandle.GetPtr()->Set(i, val);
    } else {
        VMHandle<EtsObjectArray> arrHandle(coroutine, obj->GetCoreType());
        arrHandle.GetPtr()->Set(i, EtsBoxPrimitive<T>::Create(coroutine, val));
    }
}

EtsVoid *ValueAPISetElementBoolean(EtsObject *obj, EtsLong i, EtsBoolean val)
{
    SetElement<EtsBooleanArray>(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetElementByte(EtsObject *obj, EtsLong i, EtsByte val)
{
    SetElement<EtsByteArray>(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetElementShort(EtsObject *obj, EtsLong i, EtsShort val)
{
    SetElement<EtsShortArray>(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetElementChar(EtsObject *obj, EtsLong i, EtsChar val)
{
    SetElement<EtsCharArray>(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetElementInt(EtsObject *obj, EtsLong i, EtsInt val)
{
    SetElement<EtsIntArray>(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetElementLong(EtsObject *obj, EtsLong i, EtsLong val)
{
    SetElement<EtsLongArray>(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetElementFloat(EtsObject *obj, EtsLong i, EtsFloat val)
{
    SetElement<EtsFloatArray>(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsVoid *ValueAPISetElementDouble(EtsObject *obj, EtsLong i, EtsDouble val)
{
    SetElement<EtsDoubleArray>(obj, i, val);
    return EtsVoid::GetInstance();
}

EtsObject *ValueAPIGetElementObject(EtsObject *obj, EtsLong i)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    VMHandle<EtsObjectArray> arrHandle(coroutine, obj->GetCoreType());

    return arrHandle.GetPtr()->Get(i);
}

template <typename P>
typename P::ValueType GetElement(EtsObject *obj, EtsLong i)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);
    auto typeClass = obj->GetClass();
    if (!typeClass->GetComponentType()->IsBoxedClass()) {
        VMHandle<P> arrHandle(coroutine, obj->GetCoreType());
        return arrHandle.GetPtr()->Get(i);
    }
    VMHandle<EtsObjectArray> arrHandle(coroutine, obj->GetCoreType());
    auto value = EtsBoxPrimitive<typename P::ValueType>::FromCoreType(arrHandle.GetPtr()->Get(i));
    return value->GetValue();
}

EtsBoolean ValueAPIGetElementBoolean(EtsObject *obj, EtsLong i)
{
    return GetElement<EtsBooleanArray>(obj, i);
}

EtsByte ValueAPIGetElementByte(EtsObject *obj, EtsLong i)
{
    return GetElement<EtsByteArray>(obj, i);
}

EtsShort ValueAPIGetElementShort(EtsObject *obj, EtsLong i)
{
    return GetElement<EtsShortArray>(obj, i);
}

EtsChar ValueAPIGetElementChar(EtsObject *obj, EtsLong i)
{
    return GetElement<EtsCharArray>(obj, i);
}

EtsInt ValueAPIGetElementInt(EtsObject *obj, EtsLong i)
{
    return GetElement<EtsIntArray>(obj, i);
}

EtsLong ValueAPIGetElementLong(EtsObject *obj, EtsLong i)
{
    return GetElement<EtsLongArray>(obj, i);
}

EtsFloat ValueAPIGetElementFloat(EtsObject *obj, EtsLong i)
{
    return GetElement<EtsFloatArray>(obj, i);
}

EtsDouble ValueAPIGetElementDouble(EtsObject *obj, EtsLong i)
{
    return GetElement<EtsDoubleArray>(obj, i);
}

}  // namespace panda::ets::intrinsics
