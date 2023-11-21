/*
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

#include "os/mem.h"
#include "utils/asan_interface.h"
#include "utils/tsan_interface.h"

#include <gtest/gtest.h>

namespace panda::test::mem {

TEST(Mem, GetNativeBytesFromMallinfoTest)
{
#if (!defined(PANDA_ASAN_ON)) && (!defined(PANDA_TSAN_ON)) && (defined(__GLIBC__) || defined(PANDA_TARGET_MOBILE))
    size_t old_bytes = panda::os::mem::GetNativeBytesFromMallinfo();
    // NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-no-malloc,clang-analyzer-unix.Malloc,modernize-loop-convert)
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    void *p1[1000U];
    for (size_t i = 0; i < 1000U; i++) {
        p1[i] = malloc(64U);
        ASSERT_NE(p1[i], nullptr);
    }
    size_t new_bytes = panda::os::mem::GetNativeBytesFromMallinfo();
    ASSERT_TRUE(new_bytes > old_bytes);

    old_bytes = new_bytes;
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    void *p2[10U];
    for (size_t i = 0; i < 10U; i++) {
        p2[i] = malloc(4U * 1024U * 1024U);
        ASSERT_NE(p2[i], nullptr);
    }
    new_bytes = panda::os::mem::GetNativeBytesFromMallinfo();
    ASSERT_TRUE(new_bytes > old_bytes);

    old_bytes = new_bytes;
    for (size_t i = 0; i < 1000U; i++) {
        free(p1[i]);
        p1[i] = nullptr;
    }
    new_bytes = panda::os::mem::GetNativeBytesFromMallinfo();
    ASSERT_TRUE(new_bytes < old_bytes);

    old_bytes = new_bytes;
    for (size_t i = 0; i < 10U; i++) {
        free(p2[i]);
        p2[i] = nullptr;
    }
    // NOLINTEND(readability-magic-numbers,cppcoreguidelines-no-malloc,clang-analyzer-unix.Malloc,modernize-loop-convert)
    new_bytes = panda::os::mem::GetNativeBytesFromMallinfo();
    ASSERT_TRUE(new_bytes < old_bytes);
#else
    size_t bytes = panda::os::mem::GetNativeBytesFromMallinfo();
    ASSERT_EQ(bytes, panda::os::mem::DEFAULT_NATIVE_BYTES_FROM_MALLINFO);
#endif
}

}  // namespace panda::test::mem
