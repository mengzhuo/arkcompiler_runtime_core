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

#include <gtest/gtest.h>

#include "get_test_class.h"
#include "ets_coroutine.h"

#include "types/ets_class.h"
#include "types/ets_method.h"

// NOLINTBEGIN(readability-magic-numbers)

namespace panda::ets::test {

class EtsClassTest : public testing::Test {
public:
    EtsClassTest()
    {
        options_.SetShouldLoadBootPandaFiles(true);
        options_.SetShouldInitializeIntrinsics(false);
        options_.SetCompilerEnableJit(false);
        options_.SetGcType("epsilon");
        options_.SetLoadRuntimes({"ets"});

        auto stdlib = std::getenv("PANDA_STD_LIB");
        if (stdlib == nullptr) {
            std::cerr << "PANDA_STD_LIB env variable should be set and point to mock_stdlib.abc" << std::endl;
            std::abort();
        }
        options_.SetBootPandaFiles({stdlib});

        Runtime::Create(options_);
    }

    ~EtsClassTest() override
    {
        Runtime::Destroy();
    }

    NO_COPY_SEMANTIC(EtsClassTest);
    NO_MOVE_SEMANTIC(EtsClassTest);

    void SetUp() override
    {
        coroutine_ = EtsCoroutine::GetCurrent();
        coroutine_->ManagedCodeBegin();
    }

    void TearDown() override
    {
        coroutine_->ManagedCodeEnd();
    }

private:
    RuntimeOptions options_;
    EtsCoroutine *coroutine_ = nullptr;
};

TEST_F(EtsClassTest, GetBase)
{
    {
        const char *source = R"(
            .language eTS
            .record A {}
            .record B <ets.extends = A> {}
            .record C <ets.extends = B> {}
        )";

        EtsClass *klassA = GetTestClass(source, "LA;");
        EtsClass *klassB = GetTestClass(source, "LB;");
        EtsClass *klassC = GetTestClass(source, "LC;");
        ASSERT_NE(klassA, nullptr);
        ASSERT_NE(klassB, nullptr);
        ASSERT_NE(klassC, nullptr);

        ASSERT_EQ(klassC->GetBase(), klassB);
        ASSERT_EQ(klassB->GetBase(), klassA);
        ASSERT_STREQ(klassA->GetBase()->GetDescriptor(), "Lstd/core/Object;");
    }
    {
        const char *source = R"(
            .language eTS
            .record ITest <ets.abstract, ets.interface>{}
        )";

        EtsClass *klassItest = GetTestClass(source, "LITest;");

        ASSERT_NE(klassItest, nullptr);
        ASSERT_TRUE(klassItest->IsInterface());
        ASSERT_EQ(klassItest->GetBase(), nullptr);
    }
}

TEST_F(EtsClassTest, GetStaticFieldsNumber)
{
    {
        const char *source = R"(
            .language eTS
            .record A {
                f32 x <static>
                i32 y <static>
                f32 z <static>
            }
        )";

        EtsClass *klass = GetTestClass(source, "LA;");
        ASSERT_NE(klass, nullptr);
        ASSERT_EQ(klass->GetStaticFieldsNumber(), 3);
        ASSERT_EQ(klass->GetInstanceFieldsNumber(), 0);
    }
    {
        const char *source = R"(
            .language eTS
            .record B {}
        )";

        EtsClass *klass = GetTestClass(source, "LB;");
        ASSERT_NE(klass, nullptr);
        ASSERT_EQ(klass->GetStaticFieldsNumber(), 0);
        ASSERT_EQ(klass->GetInstanceFieldsNumber(), 0);
    }
}

TEST_F(EtsClassTest, GetInstanceFieldsNumber)
{
    {
        const char *source = R"(
            .language eTS
            .record TestObject {}
            .record A {
                TestObject x
                TestObject y
                TestObject z
                TestObject k
            }
        )";

        EtsClass *klass = GetTestClass(source, "LA;");
        ASSERT_NE(klass, nullptr);
        ASSERT_EQ(klass->GetStaticFieldsNumber(), 0);
        ASSERT_EQ(klass->GetInstanceFieldsNumber(), 4);
    }
    {
        const char *source = R"(
            .language eTS
            .record B {
                f32 x <static>
            }
        )";

        EtsClass *klass = GetTestClass(source, "LB;");
        ASSERT_NE(klass, nullptr);
        ASSERT_EQ(klass->GetStaticFieldsNumber(), 1);
        ASSERT_EQ(klass->GetInstanceFieldsNumber(), 0);
    }
}

TEST_F(EtsClassTest, GetFieldIDByName)
{
    const char *source = R"(
        .language eTS
        .record TestObject {}
        .record Test {
            TestObject x
            i32 y
        }
    )";

    EtsClass *klass = GetTestClass(source, "LTest;");
    ASSERT_NE(klass, nullptr);

    ASSERT_EQ(klass->GetFieldIDByName("x", "F"), nullptr);
    ASSERT_NE(klass->GetFieldIDByName("x", "LTestObject;"), nullptr);
    ASSERT_EQ(klass->GetFieldIDByName("y", "Z"), nullptr);
    ASSERT_NE(klass->GetFieldIDByName("y", "I"), nullptr);

    EtsField *fieldX = klass->GetFieldIDByName("x");
    EtsField *fieldY = klass->GetFieldIDByName("y");
    ASSERT_NE(fieldX, nullptr);
    ASSERT_NE(fieldY, nullptr);
    ASSERT_FALSE(fieldX->IsStatic());
    ASSERT_FALSE(fieldY->IsStatic());

    ASSERT_EQ(fieldX, klass->GetFieldIDByOffset(fieldX->GetOffset()));
    ASSERT_EQ(fieldY, klass->GetFieldIDByOffset(fieldY->GetOffset()));
}

TEST_F(EtsClassTest, GetStaticFieldIDByName)
{
    const char *source = R"(
        .language eTS
        .record TestObject {}
        .record Test {
            TestObject x <static>
            i32 y <static>
        }
    )";

    EtsClass *klass = GetTestClass(source, "LTest;");
    ASSERT_NE(klass, nullptr);

    ASSERT_EQ(klass->GetStaticFieldIDByName("x", "F"), nullptr);
    ASSERT_NE(klass->GetStaticFieldIDByName("x", "LTestObject;"), nullptr);
    ASSERT_EQ(klass->GetStaticFieldIDByName("y", "Z"), nullptr);
    ASSERT_NE(klass->GetStaticFieldIDByName("y", "I"), nullptr);

    EtsField *fieldX = klass->GetStaticFieldIDByName("x");
    EtsField *fieldY = klass->GetStaticFieldIDByName("y");
    ASSERT_NE(fieldX, nullptr);
    ASSERT_NE(fieldY, nullptr);
    ASSERT_TRUE(fieldX->IsStatic());
    ASSERT_TRUE(fieldY->IsStatic());

    ASSERT_EQ(fieldX, klass->GetStaticFieldIDByOffset(fieldX->GetOffset()));
    ASSERT_EQ(fieldY, klass->GetStaticFieldIDByOffset(fieldY->GetOffset()));
}

TEST_F(EtsClassTest, SetAndGetStaticFieldPrimitive)
{
    const char *source = R"(
        .language eTS
        .record Test {
            i32 x <static>
            f32 y <static>
        }
    )";

    EtsClass *klass = GetTestClass(source, "LTest;");
    ASSERT_NE(klass, nullptr);

    EtsField *fieldX = klass->GetStaticFieldIDByName("x");
    EtsField *fieldY = klass->GetStaticFieldIDByName("y");
    ASSERT_NE(fieldX, nullptr);
    ASSERT_NE(fieldY, nullptr);

    int32_t nmb1 = 53;
    float nmb2 = 123.90;

    klass->SetStaticFieldPrimitive<int32_t>(fieldX, nmb1);
    klass->SetStaticFieldPrimitive<float>(fieldY, nmb2);

    ASSERT_EQ(klass->GetStaticFieldPrimitive<int32_t>(fieldX), nmb1);
    ASSERT_EQ(klass->GetStaticFieldPrimitive<float>(fieldY), nmb2);
}

TEST_F(EtsClassTest, SetAndGetStaticFieldPrimitiveByOffset)
{
    const char *source = R"(
        .language eTS
        .record Test {
            i32 x <static>
            f32 y <static>
        }
    )";

    EtsClass *klass = GetTestClass(source, "LTest;");
    ASSERT_NE(klass, nullptr);

    std::size_t fieldXOffset = klass->GetStaticFieldIDByName("x")->GetOffset();
    std::size_t fieldYOffset = klass->GetStaticFieldIDByName("y")->GetOffset();

    int32_t nmb1 = 53;
    float nmb2 = 123.90;

    klass->SetStaticFieldPrimitive<int32_t>(fieldXOffset, false, nmb1);
    klass->SetStaticFieldPrimitive<float>(fieldYOffset, false, nmb2);

    ASSERT_EQ(klass->GetStaticFieldPrimitive<int32_t>(fieldXOffset, false), nmb1);
    ASSERT_EQ(klass->GetStaticFieldPrimitive<float>(fieldYOffset, false), nmb2);
}

TEST_F(EtsClassTest, SetAndGetFieldObject)
{
    const char *source = R"(
        .language eTS
        .record Foo {}
        .record Bar {
            Foo x <static>
            Foo y <static>
        }
    )";

    EtsClass *barKlass = GetTestClass(source, "LBar;");
    EtsClass *fooKlass = GetTestClass(source, "LFoo;");
    ASSERT_NE(barKlass, nullptr);
    ASSERT_NE(fooKlass, nullptr);

    EtsObject *fooObj1 = EtsObject::Create(fooKlass);
    EtsObject *fooObj2 = EtsObject::Create(fooKlass);
    ASSERT_NE(fooObj1, nullptr);
    ASSERT_NE(fooObj2, nullptr);

    EtsField *fieldX = barKlass->GetStaticFieldIDByName("x");
    EtsField *fieldY = barKlass->GetStaticFieldIDByName("y");
    ASSERT_NE(fieldX, nullptr);
    ASSERT_NE(fieldY, nullptr);

    barKlass->SetStaticFieldObject(fieldX, fooObj1);
    barKlass->SetStaticFieldObject(fieldY, fooObj2);
    ASSERT_EQ(barKlass->GetStaticFieldObject(fieldX), fooObj1);
    ASSERT_EQ(barKlass->GetStaticFieldObject(fieldY), fooObj2);

    barKlass->SetStaticFieldObject(fieldX, fooObj2);
    barKlass->SetStaticFieldObject(fieldY, fooObj1);
    ASSERT_EQ(barKlass->GetStaticFieldObject(fieldX), fooObj2);
    ASSERT_EQ(barKlass->GetStaticFieldObject(fieldY), fooObj1);
}

TEST_F(EtsClassTest, SetAndGetFieldObjectByOffset)
{
    const char *source = R"(
        .language eTS
        .record Foo {}
        .record Bar {
            Foo x <static>
            Foo y <static>
        }
    )";

    EtsClass *barKlass = GetTestClass(source, "LBar;");
    EtsClass *fooKlass = GetTestClass(source, "LFoo;");
    ASSERT_NE(barKlass, nullptr);
    ASSERT_NE(fooKlass, nullptr);

    EtsObject *fooObj1 = EtsObject::Create(fooKlass);
    EtsObject *fooObj2 = EtsObject::Create(fooKlass);
    ASSERT_NE(fooObj1, nullptr);
    ASSERT_NE(fooObj2, nullptr);

    EtsField *fieldX = barKlass->GetStaticFieldIDByName("x");
    EtsField *fieldY = barKlass->GetStaticFieldIDByName("y");
    ASSERT_NE(fieldX, nullptr);
    ASSERT_NE(fieldY, nullptr);

    barKlass->SetStaticFieldObject(fieldX->GetOffset(), false, fooObj1);
    barKlass->SetStaticFieldObject(fieldY->GetOffset(), false, fooObj2);
    ASSERT_EQ(barKlass->GetStaticFieldObject(fieldX->GetOffset(), false), fooObj1);
    ASSERT_EQ(barKlass->GetStaticFieldObject(fieldY->GetOffset(), false), fooObj2);

    barKlass->SetStaticFieldObject(fieldX, fooObj2);
    barKlass->SetStaticFieldObject(fieldY, fooObj1);
    ASSERT_EQ(barKlass->GetStaticFieldObject(fieldX), fooObj2);
    ASSERT_EQ(barKlass->GetStaticFieldObject(fieldY), fooObj1);
}

TEST_F(EtsClassTest, GetDirectMethod)
{
    const char *source = R"(
        .language eTS
        .record A {}
        .record B <ets.extends = A> {}
        .record TestObject {}

        .function void A.foo1() {
            return.void
        }
        .function TestObject B.foo2(i32 a0, i32 a1, f32 a2, f64 a3, f32 a4) {
            return
        }
    )";

    EtsClass *klassA = GetTestClass(source, "LA;");
    EtsClass *klassB = GetTestClass(source, "LB;");
    ASSERT_NE(klassA, nullptr);
    ASSERT_NE(klassB, nullptr);

    EtsMethod *methodFoo1 = klassA->GetDirectMethod("foo1", ":V");
    ASSERT_NE(methodFoo1, nullptr);
    ASSERT_TRUE(!strcmp(methodFoo1->GetName(), "foo1"));

    EtsMethod *methodFoo2 = klassB->GetDirectMethod("foo2", "IIFDF:LTestObject;");
    ASSERT_NE(methodFoo2, nullptr);
    ASSERT_TRUE(!strcmp(methodFoo2->GetName(), "foo2"));

    // GetDirectMethod can't find method from base class
    EtsMethod *methodFoo1FromKlassB = klassB->GetDirectMethod("foo1", ":V");
    ASSERT_EQ(methodFoo1FromKlassB, nullptr);
}

TEST_F(EtsClassTest, GetMethod)
{
    const char *source = R"(
        .language eTS
        .record A {}
        .record B <ets.extends = A> {}
        .record TestObject {}

        .function void A.foo1() {
            return.void
        }
        .function TestObject B.foo2(i32 a0, i32 a1, f32 a2, f64 a3, f32 a4) {
            return
        }
    )";

    EtsClass *klassA = GetTestClass(source, "LA;");
    EtsClass *klassB = GetTestClass(source, "LB;");
    ASSERT_NE(klassA, nullptr);
    ASSERT_NE(klassB, nullptr);

    EtsMethod *methodFoo1 = klassA->GetMethod("foo1", ":V");
    ASSERT_NE(methodFoo1, nullptr);
    ASSERT_TRUE(!strcmp(methodFoo1->GetName(), "foo1"));

    EtsMethod *methodFoo2 = klassB->GetMethod("foo2", "IIFDF:LTestObject;");
    ASSERT_NE(methodFoo2, nullptr);
    ASSERT_TRUE(!strcmp(methodFoo2->GetName(), "foo2"));

    // GetMethod can find method from base class
    EtsMethod *methodFoo1FromKlassB = klassB->GetMethod("foo1");
    ASSERT_NE(methodFoo1FromKlassB, nullptr);
    ASSERT_TRUE(!strcmp(methodFoo1FromKlassB->GetName(), "foo1"));
}

static void TestGetPrimitiveClass(const char *primitiveName)
{
    EtsString *inputName = EtsString::CreateFromMUtf8(primitiveName);
    ASSERT_NE(inputName, nullptr);

    auto *klass = EtsClass::GetPrimitiveClass(inputName);
    ASSERT_NE(klass, nullptr);

    ASSERT_TRUE(inputName->StringsAreEqual(klass->GetName()->AsObject()));
}

TEST_F(EtsClassTest, GetPrimitiveClass)
{
    TestGetPrimitiveClass("void");
    TestGetPrimitiveClass("boolean");
    TestGetPrimitiveClass("byte");
    TestGetPrimitiveClass("char");
    TestGetPrimitiveClass("short");
    TestGetPrimitiveClass("int");
    TestGetPrimitiveClass("long");
    TestGetPrimitiveClass("float");
    TestGetPrimitiveClass("double");
}

TEST_F(EtsClassTest, SetAndGetName)
{
    const char *source = R"(
        .language eTS
        .record A {}
        .record AB {}
        .record ABC {}
    )";

    EtsClass *klass = GetTestClass(source, "LA;");
    ASSERT_NE(klass, nullptr);
    ASSERT_TRUE(klass->GetName()->StringsAreEqual(EtsString::CreateFromMUtf8("A")->AsObject()));

    klass = GetTestClass(source, "LAB;");
    ASSERT_NE(klass, nullptr);
    ASSERT_TRUE(klass->GetName()->StringsAreEqual(EtsString::CreateFromMUtf8("AB")->AsObject()));

    klass = GetTestClass(source, "LABC;");
    ASSERT_NE(klass, nullptr);
    ASSERT_TRUE(klass->GetName()->StringsAreEqual(EtsString::CreateFromMUtf8("ABC")->AsObject()));

    EtsString *name = EtsString::CreateFromMUtf8("TestNameA");
    klass->SetName(name);
    ASSERT_TRUE(klass->GetName()->StringsAreEqual(name->AsObject()));

    name = EtsString::CreateFromMUtf8("TestNameB");
    klass->SetName(name);
    ASSERT_TRUE(klass->GetName()->StringsAreEqual(name->AsObject()));

    name = EtsString::CreateFromMUtf8("TestNameC");
    klass->SetName(name);
    ASSERT_TRUE(klass->GetName()->StringsAreEqual(name->AsObject()));
}

TEST_F(EtsClassTest, CompareAndSetName)
{
    const char *source = R"(
        .language eTS
        .record Test {}
    )";

    EtsClass *klass = GetTestClass(source, "LTest;");
    ASSERT_NE(klass, nullptr);

    EtsString *oldName = EtsString::CreateFromMUtf8("TestOldName");
    EtsString *newName = EtsString::CreateFromMUtf8("TestNewName");
    ASSERT_NE(oldName, nullptr);
    ASSERT_NE(newName, nullptr);

    klass->SetName(oldName);

    ASSERT_TRUE(klass->CompareAndSetName(oldName, newName));
    ASSERT_TRUE(klass->GetName()->StringsAreEqual(newName->AsObject()));

    ASSERT_TRUE(klass->CompareAndSetName(newName, oldName));
    ASSERT_TRUE(klass->GetName()->StringsAreEqual(oldName->AsObject()));
}

TEST_F(EtsClassTest, IsInSamePackage)
{
    ASSERT_TRUE(EtsClass::IsInSamePackage("LA;", "LA;"));
    ASSERT_TRUE(EtsClass::IsInSamePackage("LA/B;", "LA/B;"));
    ASSERT_TRUE(EtsClass::IsInSamePackage("LA/B/C;", "LA/B/C;"));

    ASSERT_FALSE(EtsClass::IsInSamePackage("LA/B;", "LA/B/C;"));
    ASSERT_FALSE(EtsClass::IsInSamePackage("LA/B/C;", "LA/B;"));

    {
        const char *source = R"(
            .language eTS
            .record Test {}
        )";

        EtsClass *klass = GetTestClass(source, "LTest;");
        ASSERT_NE(klass, nullptr);

        EtsArray *array1 = EtsObjectArray::Create(klass, 1000);
        EtsArray *array2 = EtsFloatArray::Create(1000);
        ASSERT_NE(array1, nullptr);
        ASSERT_NE(array2, nullptr);

        EtsClass *klass1 = array1->GetClass();
        EtsClass *klass2 = array2->GetClass();
        ASSERT_NE(klass1, nullptr);
        ASSERT_NE(klass2, nullptr);

        ASSERT_TRUE(klass1->IsInSamePackage(klass2));
    }
    {
        EtsArray *array1 = EtsFloatArray::Create(1000);
        EtsArray *array2 = EtsIntArray::Create(1000);
        ASSERT_NE(array1, nullptr);
        ASSERT_NE(array2, nullptr);

        EtsClass *klass1 = array1->GetClass();
        EtsClass *klass2 = array2->GetClass();
        ASSERT_NE(klass1, nullptr);
        ASSERT_NE(klass2, nullptr);

        ASSERT_TRUE(klass1->IsInSamePackage(klass2));
    }
    {
        const char *source = R"(
            .language eTS
            .record Test.A {}
            .record Test.B {}
            .record Fake.A {}
        )";

        EtsClass *klassTestA = GetTestClass(source, "LTest/A;");
        EtsClass *klassTestB = GetTestClass(source, "LTest/B;");
        EtsClass *klassFakeA = GetTestClass(source, "LFake/A;");
        ASSERT_NE(klassTestA, nullptr);
        ASSERT_NE(klassTestB, nullptr);
        ASSERT_NE(klassFakeA, nullptr);

        ASSERT_TRUE(klassTestA->IsInSamePackage(klassTestA));
        ASSERT_TRUE(klassTestB->IsInSamePackage(klassTestB));
        ASSERT_TRUE(klassFakeA->IsInSamePackage(klassFakeA));

        ASSERT_TRUE(klassTestA->IsInSamePackage(klassTestB));
        ASSERT_TRUE(klassTestB->IsInSamePackage(klassTestA));

        ASSERT_FALSE(klassTestA->IsInSamePackage(klassFakeA));
        ASSERT_FALSE(klassFakeA->IsInSamePackage(klassTestA));
    }
    {
        const char *source = R"(
            .language eTS
            .record A.B.C {}
            .record A.B {}
        )";

        EtsClass *klassAbc = GetTestClass(source, "LA/B/C;");
        EtsClass *klassAb = GetTestClass(source, "LA/B;");
        ASSERT_NE(klassAbc, nullptr);
        ASSERT_NE(klassAb, nullptr);

        ASSERT_FALSE(klassAbc->IsInSamePackage(klassAb));
        ASSERT_FALSE(klassAb->IsInSamePackage(klassAbc));
    }
}

TEST_F(EtsClassTest, SetAndGetSuperClass)
{
    const char *sources = R"(
        .language eTS
        .record A {}
        .record B {}
    )";

    EtsClass *klass = GetTestClass(sources, "LA;");
    EtsClass *superKlass = GetTestClass(sources, "LB;");
    ASSERT_NE(klass, nullptr);
    ASSERT_NE(superKlass, nullptr);

    ASSERT_EQ(klass->GetSuperClass(), nullptr);
    klass->SetSuperClass(superKlass);
    ASSERT_EQ(klass->GetSuperClass(), superKlass);
}

TEST_F(EtsClassTest, IsSubClass)
{
    const char *source = R"(
        .language eTS
        .record A {}
        .record B <ets.extends = A> {}
    )";
    EtsClass *klassA = GetTestClass(source, "LA;");
    EtsClass *klassB = GetTestClass(source, "LB;");
    ASSERT_NE(klassA, nullptr);
    ASSERT_NE(klassB, nullptr);

    ASSERT_TRUE(klassB->IsSubClass(klassA));

    ASSERT_TRUE(klassA->IsSubClass(klassA));
    ASSERT_TRUE(klassB->IsSubClass(klassB));
}

TEST_F(EtsClassTest, SetAndGetFlags)
{
    const char *source = R"(
        .language eTS
        .record Test {}
    )";

    EtsClass *klass = GetTestClass(source, "LTest;");
    ASSERT_NE(klass, nullptr);

    uint32_t flags = 0U | 1U << 18U | 1U << 19U | 1U << 20U | 1U << 21U | 1U << 22U;

    klass->SetFlags(flags);
    ASSERT_EQ(klass->GetFlags(), flags);
    ASSERT_TRUE(klass->IsReference());
    ASSERT_TRUE(klass->IsSoftReference());
    ASSERT_TRUE(klass->IsWeakReference());
    ASSERT_TRUE(klass->IsFinalizerReference());
    ASSERT_TRUE(klass->IsPhantomReference());
    ASSERT_TRUE(klass->IsFinalizable());
}

TEST_F(EtsClassTest, SetAndGetComponentType)
{
    const char *source = R"(
        .language eTS
        .record Test {}
    )";

    EtsClass *klass = GetTestClass(source, "LTest;");
    ASSERT_NE(klass, nullptr);

    uint32_t arrayLength = 100;
    auto *array1 = EtsObjectArray::Create(klass, arrayLength);
    auto *array2 = EtsObjectArray::Create(klass, arrayLength);

    ASSERT_NE(array1, nullptr);
    ASSERT_NE(array2, nullptr);
    ASSERT_EQ(array1->GetClass()->GetComponentType(), array2->GetClass()->GetComponentType());

    source = R"(
        .language eTS
        .record TestObject {}
    )";

    EtsClass *componentType = GetTestClass(source, "LTestObject;");
    ASSERT_NE(componentType, nullptr);

    klass->SetComponentType(componentType);
    ASSERT_EQ(klass->GetComponentType(), componentType);

    klass->SetComponentType(nullptr);
    ASSERT_EQ(klass->GetComponentType(), nullptr);
}

TEST_F(EtsClassTest, EnumerateMethods)
{
    const char *source = R"(
        .language eTS
        .record Test {}
        .function void Test.foo1() {}
        .function void Test.foo2() {}
        .function void Test.foo3() {}
        .function void Test.foo4() {}
        .function void Test.foo5() {}
    )";

    EtsClass *klass = GetTestClass(source, "LTest;");
    ASSERT_NE(klass, nullptr);

    std::size_t methodsVectorSize = 5;
    std::vector<EtsMethod *> methods(methodsVectorSize);
    std::vector<EtsMethod *> enumerateMethods(methodsVectorSize);

    methods.push_back(klass->GetMethod("foo1"));
    methods.push_back(klass->GetMethod("foo2"));
    methods.push_back(klass->GetMethod("foo3"));
    methods.push_back(klass->GetMethod("foo4"));
    methods.push_back(klass->GetMethod("foo5"));

    klass->EnumerateMethods([&enumerateMethods, &methodsVectorSize](EtsMethod *method) {
        enumerateMethods.push_back(method);
        return enumerateMethods.size() == methodsVectorSize;
    });

    for (std::size_t i = 0; i < methodsVectorSize; ++i) {
        ASSERT_EQ(methods[i], enumerateMethods[i]);
    }
}

TEST_F(EtsClassTest, EnumerateInterfaces)
{
    const char *source = R"(
        .language eTS
        .record A <ets.abstract, ets.interface> {}
        .record B <ets.abstract, ets.interface> {}
        .record C <ets.abstract, ets.interface> {}
        .record D <ets.abstract, ets.interface> {}
        .record E <ets.abstract, ets.interface> {}
        .record Test <ets.implements=A, ets.implements=B, ets.implements=C, ets.implements=D, ets.implements=E> {}
    )";

    EtsClass *klass = GetTestClass(source, "LTest;");
    ASSERT_NE(klass, nullptr);

    std::size_t interfaceVectorSize = 5;
    std::vector<EtsClass *> interfaces(interfaceVectorSize);
    std::vector<EtsClass *> enumerateInterfaces(interfaceVectorSize);

    interfaces.push_back(GetTestClass(source, "LA;"));
    interfaces.push_back(GetTestClass(source, "LB;"));
    interfaces.push_back(GetTestClass(source, "LC;"));
    interfaces.push_back(GetTestClass(source, "LD;"));
    interfaces.push_back(GetTestClass(source, "LE;"));

    klass->EnumerateInterfaces([&enumerateInterfaces, &interfaceVectorSize](EtsClass *interface) {
        enumerateInterfaces.push_back(interface);
        return enumerateInterfaces.size() == interfaceVectorSize;
    });

    for (std::size_t i = 0; i < interfaceVectorSize; ++i) {
        ASSERT_EQ(interfaces[i], enumerateInterfaces[i]);
    }
}

}  // namespace panda::ets::test

// NOLINTEND(readability-magic-numbers)
