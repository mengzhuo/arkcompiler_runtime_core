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

#ifndef PANDA_RUNTIME_INTERPRETER_ARCH_GLOBAL_REGS_H_
#define PANDA_RUNTIME_INTERPRETER_ARCH_GLOBAL_REGS_H_

#if defined(PANDA_TARGET_ARM64)
#include "aarch64/global_regs.h"
#else
#error "Unsupported target arch"
#endif

#endif  // PANDA_RUNTIME_INTERPRETER_ARCH_GLOBAL_REGS_H_
