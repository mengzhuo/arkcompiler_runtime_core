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

#ifndef PANDA_RUNTIME_DEFAULT_DEBUGGER_AGENT_H
#define PANDA_RUNTIME_DEFAULT_DEBUGGER_AGENT_H

#include "runtime/include/loadable_agent.h"
#include "runtime/include/runtime.h"

namespace panda {
class DefaultDebuggerAgent : public LibraryAgent, public LibraryAgentLoader<DefaultDebuggerAgent, false> {
public:
    explicit DefaultDebuggerAgent(os::memory::Mutex &mutex);

    bool Load() override;
    bool Unload() override;

private:
    bool CallLoadCallback(void *resolved_function) override;
    bool CallUnloadCallback(void *resolved_function) override;

    Runtime::DebugSessionHandle debug_session_;
};
}  // namespace panda

#endif  // PANDA_RUNTIME_DEFAULT_DEBUGGER_AGENT_H
