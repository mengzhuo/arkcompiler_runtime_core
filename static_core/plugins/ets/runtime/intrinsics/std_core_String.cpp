/**
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <cstdint>
#include <regex>
#include "include/mem/panda_string.h"
#include "intrinsics.h"
#include "libpandabase/utils/logger.h"
#include "macros.h"
#include "napi/ets_napi.h"
#include "runtime/handle_scope-inl.h"
#include "runtime/entrypoints/string_index_of.h"
#include "plugins/ets/runtime/types/ets_string.h"
#include "plugins/ets/runtime/ets_exceptions.h"
#include "plugins/ets/runtime/ets_errors.h"
#include "plugins/ets/runtime/ets_language_context.h"
#include "plugins/ets/runtime/ets_panda_file_items.h"

#include "unicode/locid.h"
#include "unicode/coll.h"
#include "unicode/unistr.h"
#include "unicode/normalizer2.h"
#include "utils/span.h"

using icu::Normalizer2;

namespace ark::ets::intrinsics {

constexpr const uint32_t CHAR0X1FFC00 = 0x1ffc00;
constexpr const uint16_t CHAR0XD800 = 0xd800;
constexpr const uint16_t CHAR0XDC00 = 0xdc00;

EtsCharArray *StdCoreStringGetChars(EtsString *s, ets_int begin, ets_int end)
{
    ASSERT(s != nullptr);
    ets_int length = s->GetLength();
    if (UNLIKELY(begin > end)) {
        ark::ThrowStringIndexOutOfBoundsException(begin, length);
        return nullptr;
    }
    if (UNLIKELY(begin > length || begin < 0)) {
        ark::ThrowStringIndexOutOfBoundsException(begin, length);
        return nullptr;
    }
    if (UNLIKELY(end > length)) {
        ark::ThrowStringIndexOutOfBoundsException(end, length);
        return nullptr;
    }

    auto thread = ManagedThread::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(thread);
    VMHandle<coretypes::String> sHandle(thread, s->GetCoreType());
    ets_int n = end - begin;
    EtsCharArray *charArray = EtsCharArray::Create(n);
    if (charArray == nullptr || n == 0) {
        return charArray;
    }
    Span<ets_char> out(charArray->GetData<ets_char>(), charArray->GetLength());
    sHandle.GetPtr()->CopyDataRegionUtf16(&out[0], begin, charArray->GetLength(), sHandle.GetPtr()->GetLength());
    return charArray;
}

EtsString *StdCoreStringSubstring(EtsString *str, ets_int begin, ets_int end)
{
    ASSERT(str != nullptr);
    auto indexes = coretypes::String::NormalizeSubStringIndexes(begin, end, str->GetCoreType());
    if (UNLIKELY(indexes.first == 0 && indexes.second == str->GetLength())) {
        return str;
    }
    ets_int substrLength = indexes.second - indexes.first;
    return EtsString::FastSubString(str, static_cast<uint32_t>(indexes.first), static_cast<uint32_t>(substrLength));
}

uint16_t StdCoreStringCharAt(EtsString *s, int32_t index)
{
    ASSERT(s != nullptr);

    int32_t length = s->GetLength();

    if (UNLIKELY(index >= length || index < 0)) {
        ark::ThrowStringIndexOutOfBoundsException(index, length);
        return 0;
    }

    if (s->IsUtf16()) {
        Span<uint16_t> sp(s->GetDataUtf16(), length);
        return sp[index];
    }

    Span<uint8_t> sp(s->GetDataMUtf8(), length);
    return sp[index];
}

int32_t StdCoreStringGetLength(EtsString *s)
{
    ASSERT(s != nullptr);
    return s->GetLength();
}

double StdCoreStringLength(EtsString *s)
{
    ASSERT(s != nullptr);
    return static_cast<double>(s->GetLength());
}

EtsBoolean StdCoreStringIsEmpty(EtsString *s)
{
    ASSERT(s != nullptr);
    return ToEtsBoolean(s->IsEmpty());
}

uint8_t StdCoreStringEquals(EtsString *owner, EtsObject *s)
{
    if ((owner->AsObject()) == s) {
        return UINT8_C(1);
    }
    if (s == nullptr || !(s->GetClass()->IsStringClass())) {
        return UINT8_C(0);
    }
    return static_cast<uint8_t>(owner->StringsAreEqual(s));
}

EtsString *StdCoreStringMatch(EtsString *thisStr, EtsString *reg)
{
    PandaVector<uint8_t> buf;
    auto thisS = std::string(thisStr->ConvertToStringView(&buf));
    auto regex = std::string(reg->ConvertToStringView(&buf));

    std::regex e(regex);
    return EtsString::CreateFromMUtf8(std::sregex_iterator(thisS.begin(), thisS.end(), e)->str().c_str());
}

EtsString *StringNormalize(EtsString *str, const Normalizer2 *normalizer)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);

    icu::UnicodeString utf16Str;
    if (str->IsUtf16()) {
        utf16Str = icu::UnicodeString {str->GetDataUtf16(), static_cast<int32_t>(str->GetUtf16Length())};
    } else {
        utf16Str =
            icu::UnicodeString {utf::Mutf8AsCString(str->GetDataMUtf8()), static_cast<int32_t>(str->GetLength())};
    }

    UErrorCode errorCode = U_ZERO_ERROR;
    utf16Str = normalizer->normalize(utf16Str, errorCode);

    if (UNLIKELY(U_FAILURE(errorCode))) {
        std::string message = "Got error in process of normalization: '" + std::string(u_errorName(errorCode)) + "'";
        ThrowEtsError(coroutine, panda_file_items::class_descriptors::RANGE_ERROR, message);
        return nullptr;
    }

    return EtsString::CreateFromUtf16(reinterpret_cast<const uint16_t *>(utf16Str.getTerminatedBuffer()),
                                      utf16Str.length());
}

EtsString *StdCoreStringNormalizeNFC(EtsString *thisStr)
{
    UErrorCode errorCode = U_ZERO_ERROR;
    auto normalizer = Normalizer2::getNFCInstance(errorCode);
    if (UNLIKELY(U_FAILURE(errorCode))) {
        std::string message = "Cannot get NFC normalizer: '" + std::string(u_errorName(errorCode)) + "'";
        ThrowEtsError(EtsCoroutine::GetCurrent(), panda_file_items::class_descriptors::RANGE_ERROR, message);
        return nullptr;
    }
    return StringNormalize(thisStr, normalizer);
}

EtsString *StdCoreStringNormalizeNFD(EtsString *thisStr)
{
    UErrorCode errorCode = U_ZERO_ERROR;
    auto normalizer = Normalizer2::getNFDInstance(errorCode);
    if (UNLIKELY(U_FAILURE(errorCode))) {
        std::string message = "Cannot get NFD normalizer: '" + std::string(u_errorName(errorCode)) + "'";
        ThrowEtsError(EtsCoroutine::GetCurrent(), panda_file_items::class_descriptors::RANGE_ERROR, message);
        return nullptr;
    }
    return StringNormalize(thisStr, normalizer);
}

EtsString *StdCoreStringNormalizeNFKC(EtsString *thisStr)
{
    UErrorCode errorCode = U_ZERO_ERROR;
    auto normalizer = Normalizer2::getNFKCInstance(errorCode);
    if (UNLIKELY(U_FAILURE(errorCode))) {
        std::string message = "Cannot get NFKC normalizer: '" + std::string(u_errorName(errorCode)) + "'";
        ThrowEtsError(EtsCoroutine::GetCurrent(), panda_file_items::class_descriptors::RANGE_ERROR, message);
        return nullptr;
    }
    return StringNormalize(thisStr, normalizer);
}

EtsString *StdCoreStringNormalizeNFKD(EtsString *thisStr)
{
    UErrorCode errorCode = U_ZERO_ERROR;
    auto normalizer = Normalizer2::getNFKDInstance(errorCode);
    if (UNLIKELY(U_FAILURE(errorCode))) {
        std::string message = "Cannot get NFKD normalizer: '" + std::string(u_errorName(errorCode)) + "'";
        ThrowEtsError(EtsCoroutine::GetCurrent(), panda_file_items::class_descriptors::RANGE_ERROR, message);
        return nullptr;
    }
    return StringNormalize(thisStr, normalizer);
}

uint8_t StdCoreStringIsWellFormed(EtsString *thisStr)
{
    if (!thisStr->IsUtf16()) {
        return UINT8_C(1);
    }
    auto length = thisStr->GetUtf16Length();
    auto codeUnits = Span<uint16_t>(thisStr->GetDataUtf16(), length);
    for (size_t i = 0; i < length; ++i) {
        uint16_t codeUnit = codeUnits[i];
        if ((codeUnit & CHAR0X1FFC00) == CHAR0XD800) {
            // Code unit is a leading surrogate
            if (i == length - 1) {
                return UINT8_C(0);
            }
            // Is not trail surrogate
            if ((codeUnits[i + 1] & CHAR0X1FFC00) != CHAR0XDC00) {
                return UINT8_C(0);
            }
            // Skip the paired trailing surrogate
            ++i;
            // Is trail surrogate
        } else if ((codeUnit & CHAR0X1FFC00) == CHAR0XDC00) {
            return UINT8_C(0);
        }
    }
    return UINT8_C(1);
}

EtsString *ToLowerCase(EtsString *thisStr, const icu::Locale &locale)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);

    icu::UnicodeString utf16Str;
    if (thisStr->IsUtf16()) {
        utf16Str = icu::UnicodeString {thisStr->GetDataUtf16(), static_cast<int32_t>(thisStr->GetUtf16Length())};
    } else {
        utf16Str = icu::UnicodeString {utf::Mutf8AsCString(thisStr->GetDataMUtf8()),
                                       static_cast<int32_t>(thisStr->GetLength())};
    }
    auto res = utf16Str.toLower(locale);
    return EtsString::CreateFromUtf16(reinterpret_cast<const uint16_t *>(res.getTerminatedBuffer()), res.length());
}

EtsString *ToUpperCase(EtsString *thisStr, const icu::Locale &locale)
{
    auto coroutine = EtsCoroutine::GetCurrent();
    [[maybe_unused]] HandleScope<ObjectHeader *> scope(coroutine);

    icu::UnicodeString utf16Str;
    if (thisStr->IsUtf16()) {
        utf16Str = icu::UnicodeString {thisStr->GetDataUtf16(), static_cast<int32_t>(thisStr->GetUtf16Length())};
    } else {
        utf16Str = icu::UnicodeString {utf::Mutf8AsCString(thisStr->GetDataMUtf8()),
                                       static_cast<int32_t>(thisStr->GetLength())};
    }
    auto res = utf16Str.toUpper(locale);
    return EtsString::CreateFromUtf16(reinterpret_cast<const uint16_t *>(res.getTerminatedBuffer()), res.length());
}

UErrorCode ParseSingleBCP47LanguageTag(EtsString *langTag, icu::Locale &locale)
{
    if (langTag == nullptr) {
        locale = icu::Locale::getDefault();
        return U_ZERO_ERROR;
    }

    PandaVector<uint8_t> buf;
    std::string_view locTag = langTag->ConvertToStringView(&buf);
    icu::StringPiece sp {locTag.data(), static_cast<int32_t>(locTag.size())};
    UErrorCode status = U_ZERO_ERROR;
    locale = icu::Locale::forLanguageTag(sp, status);
    return status;
}

EtsString *StdCoreStringToUpperCase(EtsString *thisStr)
{
    return ToUpperCase(thisStr, icu::Locale::getDefault());
}

EtsString *StdCoreStringToLowerCase(EtsString *thisStr)
{
    return ToLowerCase(thisStr, icu::Locale::getDefault());
}

EtsString *StdCoreStringToLocaleUpperCase(EtsString *thisStr, EtsString *langTag)
{
    ASSERT(langTag != nullptr);

    icu::Locale locale;
    auto localeParseStatus = ParseSingleBCP47LanguageTag(langTag, locale);
    if (UNLIKELY(U_FAILURE(localeParseStatus))) {
        auto message = "Language tag '" + ConvertToString(langTag->GetCoreType()) + "' is invalid or not supported";
        ThrowEtsError(EtsCoroutine::GetCurrent(), panda_file_items::class_descriptors::RANGE_ERROR, message);
        return nullptr;
    }
    return ToUpperCase(thisStr, locale);
}

EtsString *StdCoreStringToLocaleLowerCase(EtsString *thisStr, EtsString *langTag)
{
    ASSERT(langTag != nullptr);

    icu::Locale locale;
    auto localeParseStatus = ParseSingleBCP47LanguageTag(langTag, locale);
    if (UNLIKELY(U_FAILURE(localeParseStatus))) {
        auto message = "Language tag '" + ConvertToString(langTag->GetCoreType()) + "' is invalid or not supported";
        ThrowEtsError(EtsCoroutine::GetCurrent(), panda_file_items::class_descriptors::RANGE_ERROR, message);
        return nullptr;
    }
    return ToLowerCase(thisStr, locale);
}

ets_double StdCoreStringLocaleCmp(EtsString *thisStr, EtsString *cmpStr, EtsString *langTag)
{
    ASSERT(thisStr != nullptr && cmpStr != nullptr);

    icu::Locale locale;
    auto status = ParseSingleBCP47LanguageTag(langTag, locale);
    if (UNLIKELY(U_FAILURE(status))) {
        auto message = "Language tag '" + ConvertToString(langTag->GetCoreType()) + "' is invalid or not supported";
        ThrowEtsError(EtsCoroutine::GetCurrent(), panda_file_items::class_descriptors::RANGE_ERROR, message);
        return 0;
    }

    icu::UnicodeString source;
    if (thisStr->IsUtf16()) {
        source = icu::UnicodeString {thisStr->GetDataUtf16(), static_cast<int32_t>(thisStr->GetUtf16Length())};
    } else {
        source = icu::UnicodeString {utf::Mutf8AsCString(thisStr->GetDataMUtf8()),
                                     static_cast<int32_t>(thisStr->GetLength())};
    }
    icu::UnicodeString target;
    if (cmpStr->IsUtf16()) {
        target = icu::UnicodeString {cmpStr->GetDataUtf16(), static_cast<int32_t>(cmpStr->GetUtf16Length())};
    } else {
        target =
            icu::UnicodeString {utf::Mutf8AsCString(cmpStr->GetDataMUtf8()), static_cast<int32_t>(cmpStr->GetLength())};
    }
    status = U_ZERO_ERROR;
    std::unique_ptr<icu::Collator> myCollator(icu::Collator::createInstance(locale, status));
    if (UNLIKELY(U_FAILURE(status))) {
        icu::UnicodeString dispName;
        locale.getDisplayName(dispName);
        std::string localeName;
        dispName.toUTF8String(localeName);
        LOG(FATAL, ETS) << "Failed to create the collator for " << localeName;
    }
    return myCollator->compare(source, target);
}

ets_int StdCoreStringIndexOfAfter(EtsString *s, uint16_t ch, ets_int fromIndex)
{
    return ark::intrinsics::StringIndexOfU16(s, ch, fromIndex);
}

ets_int StdCoreStringIndexOf(EtsString *s, uint16_t ch)
{
    return StdCoreStringIndexOfAfter(s, ch, 0);
}

ets_int StdCoreStringIndexOfString(EtsString *thisStr, EtsString *patternStr, ets_int fromIndex)
{
    ASSERT(thisStr != nullptr && patternStr != nullptr);
    return thisStr->GetCoreType()->IndexOf(patternStr->GetCoreType(), fromIndex);
}

ets_int StdCoreStringLastIndexOfString(EtsString *thisStr, EtsString *patternStr, ets_int fromIndex)
{
    ASSERT(thisStr != nullptr && patternStr != nullptr);
    // "abc".lastIndexOf("ab", -10) will return 0
    return thisStr->GetCoreType()->LastIndexOf(patternStr->GetCoreType(), std::max(fromIndex, 0));
}

ets_int StdCoreStringCodePointToChar(ets_int codePoint)
{
    icu::UnicodeString uniStr((UChar32)codePoint);
    uint32_t ret = bit_cast<uint16_t>(uniStr.charAt(0));
    // if codepoint contains a surrogate pair
    // encode it into int with higher bits being second char
    if (uniStr.length() > 1) {
        constexpr uint32_t BITS_IN_CHAR = 16;
        ret |= static_cast<uint32_t>(bit_cast<uint16_t>(uniStr.charAt(1))) << BITS_IN_CHAR;
    }
    return bit_cast<ets_int>(ret);
}

int32_t StdCoreStringHashCode(EtsString *thisStr)
{
    ASSERT(thisStr != nullptr);
    return thisStr->GetCoreType()->GetHashcode();
}

EtsBoolean StdCoreStringIsCompressed(EtsString *thisStr)
{
    ASSERT(thisStr != nullptr);
    return ToEtsBoolean(thisStr->GetCoreType()->IsMUtf8());
}

}  // namespace ark::ets::intrinsics
