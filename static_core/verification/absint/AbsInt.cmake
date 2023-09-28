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

set(ABSINT_SOURCES
    ${VERIFICATION_SOURCES_DIR}/absint/absint.cpp
    ${VERIFICATION_SOURCES_DIR}/absint/abs_int_inl.cpp
)

set(ABSINT_TESTS_SOURCES
    ${VERIFICATION_SOURCES_DIR}/absint/tests/reg_context_test.cpp
    ${VERIFICATION_SOURCES_DIR}/absint/tests/exec_context_test.cpp
)

set_source_files_properties(${VERIFICATION_SOURCES_DIR}/absint/absint.cpp
    PROPERTIES COMPILE_FLAGS -fno-threadsafe-statics)

set_source_files_properties(${VERIFICATION_SOURCES_DIR}/absint/abs_int_inl.cpp
    PROPERTIES COMPILE_FLAGS -fno-threadsafe-statics)
