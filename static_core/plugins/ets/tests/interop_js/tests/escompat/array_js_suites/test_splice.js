/**
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

const { etsVm, getTestModule } = require("escompat.test.js")

const ets_mod = getTestModule("escompat_test");
const GCJSRuntimeCleanup = ets_mod.getFunction("GCJSRuntimeCleanup");
const FooClass = ets_mod.getClass("FooClass");
const CreateEtsSample = ets_mod.getFunction("Array_CreateEtsSample");
const TestJSSplice = ets_mod.getFunction("Array_TestJSSplice");

{   // Test JS Array<FooClass>
    TestJSSplice(new Array(new FooClass("zero"), new FooClass("one")));
}

{   // Test ETS Array<Object>
    let arr = CreateEtsSample();
    arr.push("spliced");
    ASSERT_EQ(arr.length(), 3);
    arr.splice(1, 1);
    ASSERT_EQ(arr.at(0), 123);
    ASSERT_EQ(arr.at(1), "spliced");
    ASSERT_EQ(arr.length(), 2);

    let arr1 = CreateEtsSample();
    arr1.push("spliced");
    ASSERT_EQ(arr1.length(), 3);
    // TODO(oignatenko) uncomment below after interop will be supported for this method signature
    // arr1.splice(1);
    // ASSERT_EQ(arr1.at(0), 123);
    // ASSERT_EQ(arr1.length(), 1);
}

GCJSRuntimeCleanup();
