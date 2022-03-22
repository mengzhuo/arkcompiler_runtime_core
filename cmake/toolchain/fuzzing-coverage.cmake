# Copyright (c) 2021-2022 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This target is to collect coverage after fuzzing on special fuzzing targets

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)

set(ENV{AFL_CC_COMPILER} "GCC")
set(PANDA_ENABLE_CLANG_TIDY false CACHE BOOL "Enable clang-tidy checks during compilation" FORCE)

set(PANDA_ENABLE_AFL true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized -fprofile-arcs -ftest-coverage")
