..
    Copyright (c) 2021-2024 Huawei Device Co., Ltd.
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

.. _Implementation Details:

Implementation Details
######################

.. meta:
    frontend_status: Partly
    todo: Implement Type.for in stdlib

Important implementation details are discussed in this section.

.. _Import Path Lookup:

Import Path Lookup
******************

.. meta:
    frontend_status: Done

If an import path ``<some path>/name`` is resolved to a path in the folder
"name", then  the compiler executes the following lookup sequence:

-   If the folder contains the file ``index.ets``, then this file is imported
    as a separate module written in |LANG|;

-   If the folder contains the file ``index.ts``, then this file is imported
    as a separated module written in |TS|;

-   Otherwise, the compiler imports the package constituted by files
    ``name/*.ets``.


.. _How to get type via reflection:

How to Get Type Via Reflection
******************************

.. meta:
    frontend_status: None

The |LANG| standard library (see :ref:`Standard Library`) provides a pseudo
generic static method ``Type.for<T>()`` to be processed by the compiler in a
specific way during compilation. A call to this method allows getting a
variable of type *Type* that represents type *T* at runtime. Type *T* can
be any valid type.

.. code-block:: typescript
   :linenos:

    let type_of_int: Type = Type.for<int>()
    let type_of_string: Type = Type.for<String>()
    let type_of_array_of_int: Type = Type.for<int[]>()
    let type_of_number: Type = Type.for<number>()
    let type_of_Object: Type = Type.for<Object>()

    class UserClass {}
    let type_of_user_class: Type = Type.for<UserClass>()

    interface SomeInterface {}
    let type_of_interface: Type = Type.for<SomeInterface>()



.. raw:: pdf

   PageBreak


