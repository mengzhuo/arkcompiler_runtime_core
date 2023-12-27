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

#include <sstream>
#include <string>
#include <string_view>
#include "include/coretypes/array.h"
#include "include/managed_thread.h"
#include "macros.h"
#include "mem/vm_handle.h"
#include "napi/ets_napi.h"
#include "runtime/handle_scope-inl.h"
#include "intrinsics.h"
#include "type.h"
#include "types/ets_array.h"
#include "types/ets_class.h"
#include "types/ets_primitives.h"
#include "types/ets_box_primitive-inl.h"
#include "types/ets_string.h"
#include "utils/json_builder.h"

namespace {
panda::JsonObjectBuilder ObjectToJSON(panda::ets::EtsObject *d);

std::string EtsCharToString(ets_char data)
{
    constexpr auto HIGH_SURROGATE_MIN = 0xD800;
    constexpr auto LOW_SURROGATE_MAX = 0xDFFF;
    if ((data < HIGH_SURROGATE_MIN) || (data > LOW_SURROGATE_MAX)) {
        return std::string(reinterpret_cast<const char *>(&data), 1);
    }
    return std::string(reinterpret_cast<const char *>(&data), 2);
}

std::string_view EtsStringToView(panda::ets::EtsString *s)
{
    auto str = std::string_view();
    if (s->IsUtf16()) {
        str = std::string_view(reinterpret_cast<const char *>(s->GetDataUtf16()), s->GetUtf16Length() - 1);
    } else {
        str = std::string_view(reinterpret_cast<const char *>(s->GetDataMUtf8()), s->GetMUtf8Length() - 1);
    }
    return str;
}

template <typename PrimArr>
void EtsPrimitiveArrayToJSON(panda::JsonArrayBuilder &jsonBuilder, PrimArr *arrPtr)
{
    ASSERT(arrPtr->GetClass()->IsArrayClass() && arrPtr->IsPrimitive());
    auto len = arrPtr->GetLength();
    for (size_t i = 0; i < len; ++i) {
        jsonBuilder.Add(arrPtr->Get(i));
    }
}

template <>
void EtsPrimitiveArrayToJSON(panda::JsonArrayBuilder &jsonBuilder, panda::ets::EtsBooleanArray *arrPtr)
{
    ASSERT(arrPtr->IsPrimitive());
    auto len = arrPtr->GetLength();
    for (size_t i = 0; i < len; ++i) {
        if (arrPtr->Get(i) != 0) {
            jsonBuilder.Add(true);
        } else {
            jsonBuilder.Add(false);
        }
    }
}

template <>
void EtsPrimitiveArrayToJSON(panda::JsonArrayBuilder &jsonBuilder, panda::ets::EtsCharArray *arrPtr)
{
    ASSERT(arrPtr->IsPrimitive());
    auto len = arrPtr->GetLength();
    for (size_t i = 0; i < len; ++i) {
        auto data = arrPtr->Get(i);
        jsonBuilder.Add(EtsCharToString(data));
    }
}

panda::JsonArrayBuilder EtsArrayToJSON(panda::ets::EtsArray *arrPtr)
{
    auto jsonBuilder = panda::JsonArrayBuilder();
    if (arrPtr->IsPrimitive()) {
        panda::panda_file::Type::TypeId componentType =
            arrPtr->GetCoreType()->ClassAddr<panda::Class>()->GetComponentType()->GetType().GetId();
        switch (componentType) {
            case panda::panda_file::Type::TypeId::U1: {
                EtsPrimitiveArrayToJSON(jsonBuilder, reinterpret_cast<panda::ets::EtsBooleanArray *>(arrPtr));
                break;
            }
            case panda::panda_file::Type::TypeId::I8: {
                EtsPrimitiveArrayToJSON(jsonBuilder, reinterpret_cast<panda::ets::EtsByteArray *>(arrPtr));
                break;
            }
            case panda::panda_file::Type::TypeId::I16: {
                EtsPrimitiveArrayToJSON(jsonBuilder, reinterpret_cast<panda::ets::EtsShortArray *>(arrPtr));
                break;
            }
            case panda::panda_file::Type::TypeId::U16: {
                EtsPrimitiveArrayToJSON(jsonBuilder, reinterpret_cast<panda::ets::EtsCharArray *>(arrPtr));
                break;
            }
            case panda::panda_file::Type::TypeId::I32: {
                EtsPrimitiveArrayToJSON(jsonBuilder, reinterpret_cast<panda::ets::EtsIntArray *>(arrPtr));
                break;
            }
            case panda::panda_file::Type::TypeId::F32: {
                EtsPrimitiveArrayToJSON(jsonBuilder, reinterpret_cast<panda::ets::EtsFloatArray *>(arrPtr));
                break;
            }
            case panda::panda_file::Type::TypeId::F64: {
                EtsPrimitiveArrayToJSON(jsonBuilder, reinterpret_cast<panda::ets::EtsDoubleArray *>(arrPtr));
                break;
            }
            case panda::panda_file::Type::TypeId::I64: {
                EtsPrimitiveArrayToJSON(jsonBuilder, reinterpret_cast<panda::ets::EtsLongArray *>(arrPtr));
                break;
            }
            case panda::panda_file::Type::TypeId::U8:
            case panda::panda_file::Type::TypeId::U32:
            case panda::panda_file::Type::TypeId::U64:
            case panda::panda_file::Type::TypeId::REFERENCE:
            case panda::panda_file::Type::TypeId::TAGGED:
            case panda::panda_file::Type::TypeId::INVALID:
            case panda::panda_file::Type::TypeId::VOID:
                UNREACHABLE();
                break;
        }
    } else {
        auto arrObjPtr = reinterpret_cast<panda::ets::EtsObjectArray *>(arrPtr);
        auto len = arrObjPtr->GetLength();
        for (size_t i = 0; i < len; ++i) {
            auto d = arrObjPtr->Get(i);
            auto dCls = d->GetClass();
            auto typeDesc = dCls->GetDescriptor();
            if (dCls->IsStringClass()) {
                auto sPtr = reinterpret_cast<panda::ets::EtsString *>(d);
                jsonBuilder.Add(EtsStringToView(sPtr));
            } else if (dCls->IsArrayClass()) {
                jsonBuilder.Add([d](panda::JsonArrayBuilder &x) {
                    x = EtsArrayToJSON(reinterpret_cast<panda::ets::EtsArray *>(d));
                });
            } else if (dCls->IsBoxedClass()) {
                if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_BOOLEAN) {
                    jsonBuilder.Add(panda::ets::EtsBoxPrimitive<panda::ets::EtsBoolean>::FromCoreType(d)->GetValue());
                } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_BYTE) {
                    jsonBuilder.Add(panda::ets::EtsBoxPrimitive<panda::ets::EtsByte>::FromCoreType(d)->GetValue());
                } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_CHAR) {
                    jsonBuilder.Add(panda::ets::EtsBoxPrimitive<panda::ets::EtsChar>::FromCoreType(d)->GetValue());
                } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_SHORT) {
                    jsonBuilder.Add(panda::ets::EtsBoxPrimitive<panda::ets::EtsShort>::FromCoreType(d)->GetValue());
                } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_INT) {
                    jsonBuilder.Add(panda::ets::EtsBoxPrimitive<panda::ets::EtsInt>::FromCoreType(d)->GetValue());
                } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_LONG) {
                    jsonBuilder.Add(panda::ets::EtsBoxPrimitive<panda::ets::EtsLong>::FromCoreType(d)->GetValue());
                } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_FLOAT) {
                    jsonBuilder.Add(panda::ets::EtsBoxPrimitive<panda::ets::EtsFloat>::FromCoreType(d)->GetValue());
                } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_DOUBLE) {
                    jsonBuilder.Add(panda::ets::EtsBoxPrimitive<panda::ets::EtsDouble>::FromCoreType(d)->GetValue());
                } else {
                    UNREACHABLE();
                }
            } else {
                jsonBuilder.Add([d](panda::JsonObjectBuilder &x) { x = ObjectToJSON(d); });
            }
        }
    }
    return jsonBuilder;
}

void AddFieldsToJSON(panda::JsonObjectBuilder &curJson, const panda::Span<panda::Field> &fields,
                     panda::ets::EtsObject *d)
{
    for (const auto &f : fields) {
        ASSERT(f.IsStatic() == false);
        auto fieldName = reinterpret_cast<const char *>(f.GetName().data);

        switch (f.GetTypeId()) {
            case panda::panda_file::Type::TypeId::U1:
                curJson.AddProperty(fieldName, d->GetFieldPrimitive<bool>(f.GetOffset()));
                break;
            case panda::panda_file::Type::TypeId::I8:
                curJson.AddProperty(fieldName, d->GetFieldPrimitive<int8_t>(f.GetOffset()));
                break;
            case panda::panda_file::Type::TypeId::I16:
                curJson.AddProperty(fieldName, d->GetFieldPrimitive<int16_t>(f.GetOffset()));
                break;
            case panda::panda_file::Type::TypeId::U16:
                curJson.AddProperty(fieldName, EtsCharToString(d->GetFieldPrimitive<ets_char>(f.GetOffset())));
                break;
            case panda::panda_file::Type::TypeId::I32:
                curJson.AddProperty(fieldName, d->GetFieldPrimitive<int32_t>(f.GetOffset()));
                break;
            case panda::panda_file::Type::TypeId::F32:
                curJson.AddProperty(fieldName, d->GetFieldPrimitive<float>(f.GetOffset()));
                break;
            case panda::panda_file::Type::TypeId::F64:
                curJson.AddProperty(fieldName, d->GetFieldPrimitive<double>(f.GetOffset()));
                break;
            case panda::panda_file::Type::TypeId::I64:
                curJson.AddProperty(fieldName, d->GetFieldPrimitive<int64_t>(f.GetOffset()));
                break;
            case panda::panda_file::Type::TypeId::REFERENCE: {
                auto *fPtr = d->GetFieldObject(f.GetOffset());
                if (fPtr != nullptr) {
                    auto fCls = fPtr->GetClass();
                    auto typeDesc = fCls->GetDescriptor();
                    if (fCls->IsStringClass()) {
                        auto sPtr = reinterpret_cast<panda::ets::EtsString *>(fPtr);
                        curJson.AddProperty(fieldName, EtsStringToView(sPtr));
                    } else if (fCls->IsArrayClass()) {
                        auto aPtr = reinterpret_cast<panda::ets::EtsArray *>(fPtr);
                        curJson.AddProperty(fieldName,
                                            [aPtr](panda::JsonArrayBuilder &x) { x = EtsArrayToJSON(aPtr); });
                    } else if (fCls->IsBoxedClass()) {
                        if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_BOOLEAN) {
                            curJson.AddProperty(
                                fieldName, static_cast<bool>(
                                               panda::ets::EtsBoxPrimitive<panda::ets::EtsBoolean>::FromCoreType(fPtr)
                                                   ->GetValue()));
                        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_BYTE) {
                            curJson.AddProperty(
                                fieldName,
                                panda::ets::EtsBoxPrimitive<panda::ets::EtsByte>::FromCoreType(fPtr)->GetValue());
                        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_CHAR) {
                            curJson.AddProperty(
                                fieldName,
                                panda::ets::EtsBoxPrimitive<panda::ets::EtsChar>::FromCoreType(fPtr)->GetValue());
                        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_SHORT) {
                            curJson.AddProperty(
                                fieldName,
                                panda::ets::EtsBoxPrimitive<panda::ets::EtsShort>::FromCoreType(fPtr)->GetValue());
                        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_INT) {
                            curJson.AddProperty(
                                fieldName,
                                panda::ets::EtsBoxPrimitive<panda::ets::EtsInt>::FromCoreType(fPtr)->GetValue());
                        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_LONG) {
                            curJson.AddProperty(
                                fieldName,
                                panda::ets::EtsBoxPrimitive<panda::ets::EtsLong>::FromCoreType(fPtr)->GetValue());
                        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_FLOAT) {
                            curJson.AddProperty(
                                fieldName,
                                panda::ets::EtsBoxPrimitive<panda::ets::EtsFloat>::FromCoreType(fPtr)->GetValue());
                        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_DOUBLE) {
                            curJson.AddProperty(
                                fieldName,
                                panda::ets::EtsBoxPrimitive<panda::ets::EtsDouble>::FromCoreType(fPtr)->GetValue());
                        } else {
                            UNREACHABLE();
                        }
                    } else {
                        curJson.AddProperty(fieldName, [fPtr](panda::JsonObjectBuilder &x) { x = ObjectToJSON(fPtr); });
                    }
                } else {
                    curJson.AddProperty(fieldName, std::nullptr_t());
                }
                break;
            }
            case panda::panda_file::Type::TypeId::U8:
            case panda::panda_file::Type::TypeId::U32:
            case panda::panda_file::Type::TypeId::U64:
            case panda::panda_file::Type::TypeId::INVALID:
            case panda::panda_file::Type::TypeId::VOID:
            case panda::panda_file::Type::TypeId::TAGGED:
                UNREACHABLE();
                break;
        }
    }
}

panda::JsonObjectBuilder ObjectToJSON(panda::ets::EtsObject *d)
{
    ASSERT(d != nullptr);
    panda::ets::EtsClass *kls = d->GetClass();
    ASSERT(kls != nullptr);

    // Only instance fields are required according to JS/TS JSON.stringify behaviour
    auto curJson = panda::JsonObjectBuilder();
    kls->EnumerateBaseClasses([&](panda::ets::EtsClass *c) {
        AddFieldsToJSON(curJson, c->GetRuntimeClass()->GetInstanceFields(), d);
        return false;
    });
    return curJson;
}
}  // namespace

namespace panda::ets::intrinsics {
EtsString *EscompatJSONStringifyObj(EtsObject *d)
{
    ASSERT(d != nullptr);
    auto thread = ManagedThread::GetCurrent();
    [[maybe_unused]] auto _ = HandleScope<ObjectHeader *>(thread);
    auto dHandle = VMHandle<EtsObject>(thread, d->GetCoreType());
    auto cls = dHandle.GetPtr()->GetClass();
    auto typeDesc = cls->GetDescriptor();

    auto resString = std::string();
    if (cls->IsArrayClass()) {
        auto arr = reinterpret_cast<panda::ets::EtsArray *>(dHandle.GetPtr());
        resString = EtsArrayToJSON(reinterpret_cast<panda::ets::EtsArray *>(arr)).Build();
    } else if (cls->IsBoxedClass()) {
        std::stringstream ss;
        ss.setf(std::stringstream::boolalpha);
        if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_BOOLEAN) {
            ss << static_cast<bool>(panda::ets::EtsBoxPrimitive<panda::ets::EtsBoolean>::FromCoreType(d)->GetValue());
        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_BYTE) {
            ss << panda::ets::EtsBoxPrimitive<panda::ets::EtsByte>::FromCoreType(d)->GetValue();
        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_CHAR) {
            ss << panda::ets::EtsBoxPrimitive<panda::ets::EtsChar>::FromCoreType(d)->GetValue();
        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_SHORT) {
            ss << panda::ets::EtsBoxPrimitive<panda::ets::EtsShort>::FromCoreType(d)->GetValue();
        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_INT) {
            ss << panda::ets::EtsBoxPrimitive<panda::ets::EtsInt>::FromCoreType(d)->GetValue();
        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_LONG) {
            ss << panda::ets::EtsBoxPrimitive<panda::ets::EtsLong>::FromCoreType(d)->GetValue();
        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_FLOAT) {
            ss << panda::ets::EtsBoxPrimitive<panda::ets::EtsFloat>::FromCoreType(d)->GetValue();
        } else if (typeDesc == panda::ets::panda_file_items::class_descriptors::BOX_DOUBLE) {
            ss << panda::ets::EtsBoxPrimitive<panda::ets::EtsDouble>::FromCoreType(d)->GetValue();
        } else {
            UNREACHABLE();
        }
        resString = ss.str();
    } else {
        resString = ObjectToJSON(dHandle.GetPtr()).Build();
    }
    auto etsResString = EtsString::CreateFromUtf8(resString.data(), resString.size());
    return etsResString;
}

}  // namespace panda::ets::intrinsics
