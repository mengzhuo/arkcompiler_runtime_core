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
#ifndef VERIFIER_VERIFY_H
#define VERIFIER_VERIFY_H

#include "verifier.h"

#include <string>

bool Verify([[maybe_unused]] const std::string &input_file)
{
    panda::verifier::Verifier vf {};
    if (vf.Verify(input_file)) {
        return true;
    }

    return false;
}

#endif