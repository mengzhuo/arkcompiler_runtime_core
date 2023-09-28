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

#ifndef PANDA_VERIFICATION_PUBLIC_INTERNAL_H_
#define PANDA_VERIFICATION_PUBLIC_INTERNAL_H_

#include "verification/verification_options.h"
#include "verification/jobs/service.h"
#include "verification/config/context/context.h"

namespace panda::verifier {

struct Config {
    VerificationOptions opts;
    debug::DebugConfig debug_cfg;
};

struct Service {
    Config const *config = nullptr;
    ClassLinker *class_linker = nullptr;
    debug::DebugContext debug_ctx;
    mem::InternalAllocatorPtr allocator;
    VerifierService *verifier_service = nullptr;
};

}  // namespace panda::verifier

#endif  // PANDA_VERIFICATION_PUBLIC_INTERNAL_H_
