/**
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include <iostream>
#include "SamplerAsmTest.h"

#ifdef __cplusplus
extern "C" {
#endif

extern "C" int SumEightElements(int, int, int, int, int, int, int, int);

// NOLINTBEGIN(readability-magic-numbers, readability-named-parameter, cppcoreguidelines-pro-type-vararg,
// hicpp-signed-bitwice)

ETS_EXPORT ets_int ETS_CALL ETS_ETSGLOBAL_NativeSumEightElements([[maybe_unused]] EtsEnv *, [[maybe_unused]] ets_class)
{
    for (int j = 0; j < 200; j++) {
        for (int i = 0; i < 10'000'000; ++i) {
            int val = SumEightElements(i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6, i + 7);
            if (val != 8 * i + 28) {
                std::cerr << val << std::endl;
                return 1;
            }
        }
    }

    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-named-parameter, cppcoreguidelines-pro-type-vararg,
// hicpp-signed-bitwice)

#ifdef __cplusplus
}
#endif
