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

.. _Classes:

Classes
#######

.. meta:
    frontend_status: Done

Class declarations introduce new reference types and describe the manner
of their implementation.

Classes can be *top-level* and local (see :ref:`Local Classes And Interfaces`).

A class body contains declarations and class initializers.

Declarations can introduce class members (see :ref:`Class Members`) or class
constructors (see :ref:`Constructor Declaration`).

The body of the declaration of a member comprises the scope of a
declaration (see :ref:`Scopes`).

Class members include:

-  Fields,
-  Methods, and
-  Accessors.

.. index::
   class declaration
   reference type
   implementation
   class body
   field
   method
   accessor
   constructor
   instance
   member
   class initializer
   scope

Class members can be *declared* or *inherited*.

Every member is associated with the class declaration it is declared in.

Field, method, accessor and constructor declarations can have the following
access modifiers (see :ref:`Access Modifiers`):

-  *Public*,
-  *Protected*,
-  *Internal*, or
-  *Private*.

A newly declared field can hide a field declared in a superclass or
superinterface.

A newly declared method can hide, implement, or override a method
declared in a superclass or superinterface.

Every class defines two class-level scopes (see :ref:`Scopes`): one for
instance members, and the other for static members. This means that two members
of a class can have the same name if one is static while the other is not.

.. index::
   class declaration
   access modifier
   field declaration
   method declaration
   accessor declaration
   constructor declaration
   hiding
   implementation
   overriding
   superclass
   superinterface
   

|

.. _Class Declarations:

Class Declarations
******************

.. meta:
    frontend_status: Done

Every class declaration defines a *class type*, i.e., a new named
reference type.

The class name is specified by an *identifier* inside a class declaration.

If ``typeParameters`` are defined in a class declaration, then that class
is a *generic class* (see :ref:`Generic Declarations`).

.. index::
   class declaration
   class type
   reference type
   identifier
   generic class
   scope

.. code-block:: abnf

    classDeclaration:
        classModifier? 'class' identifier typeParameters?
          classExtendsClause? implementsClause? classBody
        ;

    classModifier:
        'abstract' | 'final'
        ;

The scope of a class declaration is specified in :ref:`Scopes`.

An example of a class is presented below:

.. code-block:: typescript
   :linenos:

    class Point {
      public x: number
      public y: number
      public constructor(x : number, y : number) {
        this.x = x
        this.y = y
      }
      public distanceBetween(other: Point): number {
        return Math.sqrt(
          (this.x - other.x) * (this.x - other.x) +
          (this.y - other.y) * (this.y - other.y)
        )
      }
      static origin = new Point(0, 0)
    }

|

.. _Abstract Classes:

Abstract Classes
================

.. meta:
    frontend_status: Done

A class with the modifier ``abstract`` is known as abstract class.
Abstract classes can be used to represent notions that are common
to some set of more concrete notions.

A :index:`compile-time error` occurs if an attempt is made to create
an instance of an abstract class:

.. code-block:: typescript
   :linenos:

   abstract class X {
      field: number
      constructor (p: number) { this.field = p }
   }
   let x = new X (666)
     // Compile-time error: Cannot create an instance of an abstract class.

Subclasses of an abstract class can be non-abstract or in turn abstract.
A non-abstract subclass of an abstract superclass can be instantiated. As a
result, a constructor for the abstract class, and field initializers
for non-static fields of that class are executed.

.. code-block:: typescript
   :linenos:

   abstract class Base {
      field: number
      constructor (p: number) { this.field = p }
   }

   class Derived extends Base {
      constructor (p: number) { super(p) }
   }

A method with the modifier ``abstract`` is considered an *abstract method*
(see :ref:`Abstract Methods`).
Abstract methods do not have bodies, i.e., they can be declared but not
implemented.

Only abstract classes can have abstract methods.
A :index:`compile-time error` occurs if a non-abstract class has 
an abstract method:

.. code-block:: typescript
   :linenos:

   class Y {
     abstract method (p: string): void
     /* Compile-time error: Abstract methods can only 
        be within an abstract class. */
   }

A :index:`compile-time error` occurs if an abstract method declaration
contains the modifiers ``final`` or ``override``.

.. index::
   modifier
   abstract
   method
   non-abstract
   class
   subclass
   instance
   instantiation
   constructor
   initializer
   non-static
   field
   execution
   implementation
   abstract method
   final

|

.. _Final Classes:

Final Classes
=============

.. meta:
    frontend_status: Done

Final classes are described in the chapter Experimental Features (see
:ref:`Final Classes Experimental`).

.. index::
   modifier
   class
   final


|

.. _Class Extension Clause:

Class Extension Clause
======================

.. meta:
    frontend_status: Done

All classes except class ``Object`` can contain the ``extends`` clause that
specifies the *base class*, or the *direct superclass* of the current class.
A class that has no *extends* clause, and is not ``Object``, is assumed to have
the ``extends`` ``Object`` clause.

A class that inherits from its superclass is the *subclass* of that superclass.

.. index::
   class
   Object
   clause
   direct superclass
   base class
   superclass

.. code-block:: abnf

    classExtendsClause:
        'extends' typeReference
        ;

A :index:`compile-time error` occurs if:

-  An ``extends`` clause appears in the definition of the class ``Object``,
   which is the top of the type hierarchy, and has no superclass.

-  The class type named by ``typeReference`` is not accessible (see
   :ref:`Scopes`).

-  The *extends* graph has a cycle.

-  typeReference refers directly, or as alias of types ``primitive``, ``enum``,
   ``union``, ``interface``, or ``function``.

-  Any type argument of ``typeReference`` is a wildcard type argument.


*Class extension* implies that a class inherits all members of the direct
superclass. Notice that private members are also inherited from the superclass
but they are not accessible within the subclass.

.. index::
   class
   Object
   superclass
   type
   enum type
   primitive type
   class type
   extends clause
   extends graph
   wildcard
   type argument
   inheritance

.. code-block:: typescript
   :linenos:

    class Base {
      // All methods are mutually accessible in the class where
          they were declared
      public publicMethod () {
        this.protectedMethod()
        this.privateMethod()
      }
      protected protectedMethod () {
        this.publicMethod()
        this.privateMethod()
      }
      private privateMethod () {
        this.publicMethod();
        this.protectedMethod()
      }
    }
    class Derived extends Base {
      foo () {
        this.publicMethod()    // OK
        this.protectedMethod() // OK
        this.privateMethod()   // compile-time error:
                               // the private method is inaccessible
      }
    }

The transitive closure of a *direct subclass* relationship is the *subclass*
relationship. Class *A* can be a subclass of class *C* if:

-  *A* is the direct subclass of *C*; or

-  *A* is a subclass of some class *B*,  which is in turn a subclass of *C*
   (i.e., the definition applies recursively).


Class *C* is a *superclass* of class *A* if *A* is its subclass.

.. index::
   transitive closure
   direct subclass
   subclass relationship
   subclass
   class

|

.. _Class Implementation Clause:

Class Implementation Clause
===========================

.. meta:
    frontend_status: Done

A class can implement one or more interfaces. Interfaces that are to
be implemented by a class are listed in the ``implements`` clause.
Interfaces listed in this clause are *direct superinterfaces*
of the class.

.. code-block:: abnf

    implementsClause:
        'implements' interfaceTypeList
        ;

    interfaceTypeList:
        typeReference (',' typeReference)*
        ;

A :index:`compile-time error` occurs if:

-  ``typeReference`` fails to name an accessible interface type (see
   :ref:`Scopes`).

-  Any type argument of ``typeReference`` is a wildcard type argument.

-  An interface is repeated as a direct superinterface in a single
   ``implements`` clause (even if that interface is named differently).

.. index::
   class declaration
   implementation
   accessible interface type
   type argument
   wildcard
   interface
   direct superinterface
   implements clause

For the class declaration ``C`` <``F``:sub:`1` ``,..., F``:sub:`n`> (:math:`n\geq{}0`,
:math:`C\neq{}Object`):

- *Direct superinterfaces* of the class type ``C`` <``F``:sub:`1` ``,..., F``:sub:`n`>
  are the types specified in the ``implements`` clause of the declaration of *C*
  (if there is an ``implements`` clause).


For the generic class declaration ``C`` <``F``:sub:`1` ``,..., F``:sub:`n`> (*n* > *0*):

-  *Direct superinterfaces* of the parameterized class type *C*
   < ``T``:sub:`1` ``,..., T``:sub:`n`> are all types *I*
   < ``U``:sub:`1`:math:`\theta{}` ``,..., U``:sub:`k`:math:`\theta{}`> if:

    - ``T``:sub:`i` (:math:`1\leq{}i\leq{}n`) is a type;
    - ``I`` <``U``:sub:`1` ``,..., U``:sub:`k`> is the direct superinterface of
      ``C`` <``F``:sub:`1` ``,..., F``:sub:`n`>; and
    - :math:`\theta{}` is the substitution [``F``:sub:`1` ``:= T``:sub:`1` ``,..., F``:sub:`n` ``:= T``:sub:`n`].

.. index::
   class declaration
   parameterized class type
   generic class
   direct superinterface
   implements clause

Interface type *I* is a superinterface of class type *C* if *I* is one of the
following:

-  Direct superinterface of *C*;
-  Superinterface of *J* which is in turn a direct superinterface of *C*
   (see :ref:`Superinterfaces and Subinterfaces` that defines superinterface
   of an interface); or
-  Superinterface of the direct superclass of *C*.


A class *implements* all its superinterfaces.

A :index:`compile-time error` occurs if a class is at the same time a
subtype of:

-  Two interface types that represent different instantiations of the same
   generic interface (see :ref:`Generic Declarations`); or
-  The instantiation of a generic interface, and a raw type that names the
   a generic interface.

.. index::
   class type
   direct superinterface
   superinterface
   interface
   superclass
   class
   subtype
   interface type
   instantiation
   generic interface
   raw type

If a class is not declared *abstract*, then:

-  Any abstract method of each direct superinterface is implemented (see
   :ref:`Overriding by Instance Methods`) by a declaration in that class.
-  The declaration of the existing method is inherited from a direct superclass,
   or a direct superinterface.


If a default method (see :ref:`Default Method Declarations`) of a 
superinterface is not inherited, then that default method can:

-  Be overridden by a class method; and
-  Behave as specified in its default body.


A single method declaration in a class is allowed to implement methods of one
or more superinterfaces.

A :index:`compile-time error` occurs if a class field and a method from one
of superinterfaces that a class implements have the same name, except when one
is static and the other is not.

.. index::
   class type
   abstract class
   abstract method
   superinterface
   implementation
   overriding
   declaration
   class field
   method declaration
   inheritance
   superclass
   compile-time error
   implementation
   method body

|

.. _Implementing Interface Properties:

Implementing Interface Properties
=================================

.. meta:
    frontend_status: Done

A class must implement all properties from all interfaces (see
:ref:`Implementing Interface Properties`) which are defined as a getter, a
setter, or both. Providing implementation for the property in the form of
a field is not necessary.

.. code-block:: typescript
   :linenos:

    interface Style {
      get color(): string
      set color(s: string)
    }

    class StyleClassOne implements Style {
      color: string = ""
    }

    class StyleClassTwo implements Style {
      private color_: string = ""

      get color(): string {
        return this.color_
      }

      set color(s: string) {
        this.color_ = s
      }
    }

.. index::
   class
   implementation
   getter
   setter
   field

|

.. _Class Body:

Class Body
**********

.. meta:
    frontend_status: Done

A *class body* can contain declarations of the following members:

-  Fields,
-  Methods,
-  Accessors,
-  Constructors, and
-  Static initializers for the class.

.. code-block:: abnf

    classBody:
        '{' 
           classBodyDeclaration* classInitializer? classBodyDeclaration*
        '}'
        ;

    classBodyDeclaration:
        accessModifier?
        ( constructorDeclaration
        | classFieldDeclaration
        | classMethodDeclaration
        | classAccessorDeclaration
        )
        ;

Declarations can be inherited or immediately declared in a class. Any
declaration within a class has a class scope. The class scope is fully
defined in :ref:`Scopes`.

.. index::
   class body
   declaration
   member
   field
   method
   accessor
   type
   class
   interface
   constructor
   class initializer
   inheritance
   scope

|

.. _Class Members:

Class Members
*************

.. meta:
    frontend_status: Done

Class members are as follows:

-  Members inherited from their direct superclass (see :ref:`Inheritance`),
   except class ``Object`` that cannot have a direct superclass.
-  Members declared in a direct superinterface (see
   :ref:`Superinterfaces and Subinterfaces`).
-  Members declared in the class body (see :ref:`Class Body`).


Class members declared *private* are not accessible to all subclasses of
the class.

.. index::
   inheritance
   member
   direct superclass
   Object
   direct superinstance
   class body
   private
   subclass

Class members declared *protected* or *public* are inherited by subclasses
that are declared in a package other than the package containing the class
declaration.

Constructors and class initializers are not members, and are not inherited.

Members can be as follows:

-  Class fields (see :ref:`Field Declarations`),
-  Methods (see :ref:`Method Declarations`), and
-  Accessors (see :ref:`Accessor Declarations`).


A *method* is defined by the following:

#. *Type parameter*, i.e., the declaration of any type parameter of the
   method member.
#. *Argument type*, i.e., the list of types of arguments applicable to the
   method member.
#. *Return type*, i.e., the return type of the method member.
#. A ``throws``/``rethrows`` clause, i.e., an indication of the ability of a
   member method to raise exceptions.

Members can be as follows:

-  Static members that are not part of class instances, and can be accessed
   by using a qualified name notation (see :ref:`Names`) anywhere the class
   name or the interface name is accessible; and
-  Non-static, or instance members that belong to any instance of the class.


All names in both static and non-static class declaration scopes (see
:ref:`Scopes`) must be unique, i.e., fields and methods cannot have the
same name.

.. index::
   class
   member
   protected
   public
   inheritance
   subclass
   package
   declaration
   constructor
   initializer
   field
   method
   accessor
   return type
   argument type
   throws clause
   rethrows clause
   type parameter
   declaration scope

|

.. _Access Modifiers:

Access Modifiers
****************

.. meta:
    frontend_status: Partly

Access modifiers define how a class member or a constructor can be accessed.
Accessibility in |LANG| can be of the following kinds:

-  Private,
-  Internal,
-  Protected, or
-  Public.

The desired accessibility of class members and constructors can be explicitly
specified by the corresponding *access modifiers*:

.. code-block:: abnf

    accessModifier:
        'private'
        | 'internal'
        | 'protected'
        | 'public'
        ;

If no explicit modifier is provided, then a class member or a constructor
is implicitly considered public by default.

.. index::
   access modifier
   member
   constructor
   private
   public
   accessibility

|

.. _Private Access Modifier:

Private Access Modifier
=======================

.. meta:
    frontend_status: Done
    todo: only parsing is implemented, but checking isn't implemented yet, need libpandafile support too

The modifier ``private`` indicates that a class member or a constructor is
accessible within its declaring class, i.e., a private member or
constructor *m* declared in a class *C* can be accessed only within the
class body of *C*:

.. code-block:: typescript
   :linenos:

    class C {
      private count: number
      getCount(): number {
        return this.count // ok
      }
    }

    function increment(c: C) {
      c.count++ // compile-time error – 'count' is private
    }

.. index::
   modifier
   private
   class member
   constructor
   accessibility
   declaring class
   class body

|

.. _Internal Access Modifier:

Internal Access Modifier
========================

.. meta:
    frontend_status: Partly
    todo: Implement in libpandafile, implement semantic, now it is parsed and ignored - #16088

The modifier ``internal`` is described in the chapter Experimental Features
(see :ref:`Internal Access Modifier Experimental`).

|

.. _Protected Access Modifier:

Protected Access Modifier
=========================

.. meta:
    frontend_status: Done

The modifier ``protected`` indicates that a class member or a constructor is
accessible only within its declaring class and the classes derived from that
declaring class. A protected member *M* declared in a class *C* can be
accessed only within the class body of *C* or of a class derived from *C*:

.. code-block:: typescript
   :linenos:

    class C {
      protected count: number
       getCount(): number {
         return this.count // ok
       }
    }

    class D extends C {
      increment() {
        this.count++ // ok, D is derived from C
      }
    }

    function increment(c: C) {
      c.count++ // compile-time error – 'count' is not accessible
    }

.. index::
   modifier
   method
   protected
   constructor
   accessibility
   class body
   declaring class

|

.. _Public Access Modifier:

Public Access Modifier
======================

.. meta:
    frontend_status: Done
    todo: spec needs to be clarified - "The only exception and panic here is that the type the member or constructor belongs to must also be accessible"

The modifier ``public`` indicates that a class member or a constructor can be
accessed everywhere, provided that the member or the constructor belongs to
a type that is also accessible.

.. index::
   modifier
   protected
   access
   public
   constructor

|

.. _Field Declarations:

Field Declarations
******************

.. meta:
    frontend_status: Done

*Field declarations* represent data members in class instances.
Fields are actually class instance variables (see :ref:`Variable Declarations`).

.. code-block:: abnf

    classFieldDeclaration:
        fieldModifier* variableDeclaration
        ;

    fieldModifier:
        'static' | 'readonly'
        ;

A :index:`compile-time error` occurs if:

-  The same field modifier is used more than once in a field declaration.
-  The name of a field declared in the body of a class declaration is already
   used for another field or method in the same declaration.


A field declared by a class with a certain name *hides* any accessible
declaration of fields if they have the same name in superclasses and
superinterfaces of the class.

.. index::
   field declaration
   data member
   class instance
   field modifier
   class declaration
   hiding
   access
   superclass
   superinterface
   class declaration body
   
If a hidden field is static, then it can be accessed with the qualification
of a superclass or of a superinterface, or with a field access expression with
the keyword ``super`` (see :ref:`Field Access Expressions`).

A class can access all non-private fields of a superclass and superinterfaces
from its direct superclass and direct superinterfaces if such non-private
fields are both:

-  Accessible (see :ref:`Scopes`) to code in the class; and
-  Not hidden by a declaration in the class.

A class can inherit more than one field or property with the same name from
its superinterfaces, or from both its superclass and superinterfaces. However,
an attempt to refer to such a field or property by its simple name within the
body of the class causes a :index:`compile-time error`.

The same field or property declaration can be inherited from an interface in
more than one way. In that case, the field or property is considered
to be inherited only once (and thus, referring to it by its simple name causes
no ambiguity).

.. index::
   qualified name
   access
   class body
   hiding
   hidden field
   static field
   field access expression
   keyword super
   superclass
   type
   inheritance
   subclass
   private
   property declaration

|

.. _Static Fields:

Static Fields
=============

.. meta:
    frontend_status: Done

There are two categories of class or interface fields as follows:

- Static fields

  Static fields are declared with the modifier ``static``. A static field
  is not part of a class instance. There is one copy of a static field
  irrespective of how many instances of the class (even if zero) are
  eventually created.

  Static fields are accessed by using a qualified name notation at any place
  the class or interface name is accessible (see :ref:`Names` if class or
  interface name is used as a qualifier).

- Instance, or non-static fields

  Instance fields belong to each instance of the class. An instance field
  is created for, and associated with a newly-created instance of a class,
  or of its superclass. An instance field is accessible via the name of the
  instance.

.. index::
   static field
   instantiation
   instance
   initialization
   class
   superclass
   non-static field

|

.. _Readonly Constant Fields:

Readonly (Constant) Fields
==========================

.. meta:
    frontend_status: Done

A field that has the modifier ``readonly`` is a *readonly field*. Changing
the value of a readonly field after initialization is not allowed.
 
Both static and non-static fields can be declared *readonly fields*.

A static readonly field must be initialized as follows:

-  By using a field initializer, or
-  As a result of a class initializer (see :ref:`Class Initializer`).


Otherwise, a :index:`compile-time error` occurs.

A non-static readonly field must be initialized as a result of
execution of any class constructor. Otherwise, a :index:`compile-time error`
occurs.

.. index::
   readonly field
   constant field
   initialization
   modifier
   static field
   non-static field
   execution
   constructor

|

.. _Field Initialization:

Field Initialization
====================

.. meta:
    frontend_status: Done

An initializer in a non-static field declaration has the semantics of
an assignment (see :ref:`Assignment`) to the declared variable.

The following rules apply to an initializer in a static field declaration:

-  A :index:`compile-time error` occurs if the initializer uses the keywords
   ``this`` or ``super`` while calling a method (see 
   :ref:`Method Call Expression`) or accessing a field (see
   :ref:`Field Access Expressions`).
-  The initializer is evaluated, and the assignment is performed only once
   when the class is initialized at runtime.

**Note**: Constant fields are initialized before all other static fields.

Constant fields initialization never uses default values (see
:ref:`Default Values for Types`).

In a non-static field declaration, an initializer is evaluated at runtime.
Its assignment is performed each time an instance of the class is created.
The initializer can use the following keywords:

-  ``this`` to access the current object methods or fields, or to refer to the
   current object;
-  ``super`` to access the methods declared in a superclass;

.. index::
   initializer
   non-static field
   field declaration
   constant field
   initialization
   keyword this
   keyword super
   assignment
   variable
   access
   superclass
   object
   assignment
   evaluation
   creation
   access
   static field
   instance
   class

Additional restrictions (as specified in :ref:`Exceptions and Errors Inside Field Initializers`)
apply to variable initializers that refer to fields which cannot be
initialized yet.

References to a field (even if the field is in the scope) can be restricted.
The rules applying to the restrictions on forward references to fields (if the
reference textually precedes the field declaration) and self-references (if
the field is used within its own initializer) are provided below.

A :index:`compile-time error` occurs in a reference to a static field *f*
declared in class or interface *C* if:

-  Such a reference is used in *C*’s static initializer (see
   :ref:`Class Initializer`) or static field initializer (see
   :ref:`Field Initialization`);
-  Such a reference is used before the declaration of *f*, or within *f*’s own
   declaration initializer;
-  No such reference is present on the left-hand side of an assignment
   expression (see :ref:`Assignment`);
-  *C* is the innermost class or interface that encloses such a reference.


A :index:`compile-time error` occurs in a reference to a non-static field *f*
declared in class *C* if such reference is:

-  Used in the non-static field initializer of *C*;
-  Used before the declaration of *f*, or within *f*’s own declaration
   initializer;
-  Not present on the left-hand side of an assignment expression (see
   :ref:`Assignment`);
-  Enclosed in *C* that is the innermost class or interface.

.. index::
   restriction
   exception
   error
   initializer
   variable
   field
   interface
   expression
   assignment
   reference
   non-static field
   static field
   innermost class
   innermost interface
   enclosing

|

.. _Method Declarations:

Method Declarations
*******************

.. meta:
    frontend_status: Done

*Methods* declare executable code that can be called:

.. code-block:: abnf

    classMethodDeclaration:
        methodOverloadSignature*
        methodModifier* typeParameters? identifier signature block?
        ;

    methodModifier:
        'abstract'
        | 'static'
        | 'final'
        | 'override'
        | 'native'
        ;

*Overloading signature* of a method allows calling a method in different ways.

The identifier of ``classMethodDeclaration`` is the method name that can be
used to refer to a method (see :ref:`Method Call Expression`).

A :index:`compile-time error` occurs if:

-  A method modifier appears more than once in a method declaration.
-  The body of a class declaration declares a method but the name of that
   method is already used for a field in the same declaration.
-  The body of a class declaration declares two same-name methods with
   override-equivalent signatures (see :ref:`Override-Equivalent Signatures`)
   as members of that body of a class declaration.

.. index::
   method declaration
   overload signature
   identifier
   method
   method modifier
   class declaration
   override-equivalent signature
   class declaration body

|

.. _Static Methods:

Static Methods
==============

.. meta:
    frontend_status: Done

A method declared in a class with the modifier ``static`` is a *static method*.

A :index:`compile-time error` occurs if:

-  A method declaration contains another modifier (``abstract``, ``final``, or
   ``override``) along with the modifier ``static``.
-  The header or body of a class method includes the name of a type parameter
   of the surrounding declaration.

Static methods are always called without reference to a particular object. As
a result, a :index:`compile-time error` occurs if keywords ``this`` or ``super``
are used inside a static method.

.. index::
   static method
   keyword this
   keyword super
   keyword abstract
   keyword final
   keyword override
   keyword static
   class method header
   class method body
   type parameter

|

.. _Instance Methods:

Instance Methods
================

.. meta:
    frontend_status: Done

A method that is not declared static is called *non-static method*, or
an *instance method*.

An instance method is always called with respect to an object that becomes
the current object the keyword ``this`` refers to during the execution
of the method body.

.. index::
   static method
   instance method
   non-static method
   keyword this
   method body

|

.. _Abstract Methods:

Abstract Methods
================

.. meta:
    frontend_status: Done

An *abstract* method declaration introduces the method as a member along
with its signature but without an implementation. An abstract method is
declared with the modifier ``abstract`` in its declaration.

Non-abstract methods can be referred to as *concrete methods*.

A :index:`compile-time error` occurs if:

-  An abstract method is declared private.
-  A method declaration contains another modifier (``static``, ``final``, or
   ``native``) along with the modifier ``abstract``.
-  The abstract method *m* declaration does not appear directly within an
   abstract class *A*.
-  Any non-abstract subclass of *A*
   (see :ref:`Abstract Classes`) does not provide an implementation
   for *m*.

An abstract method can be overridden by another abstract method declaration
provided by an abstract subclass.

A :index:`compile-time error` occurs if an abstract method overrides a
non-abstract instance method.

.. index::
   abstract method declaration
   abstract method
   non-abstract instance method
   non-abstract method
   signature
   keyword abstract
   keyword static
   keyword final
   keyword native
   private
   abstract class
   overriding


|

.. _Final Methods:

Final Methods
=============

.. meta:
    frontend_status: Done

Final methods are described in the chapter Experimental Features (see
:ref:`Native Methods Experimental`).

|

.. _Override Methods:

Overriding Methods
==================

.. meta:
    frontend_status: Done

The ``override`` modifier indicates that an instance method in a superclass is
overridden by the corresponding instance method from a subclass (see
:ref:`Overriding by Instance Methods`).

The usage of the modifier ``override`` is optional but strongly recommended as
it makes the overriding explicit.

A :index:`compile-time error` occurs if:

-  A method marked with the modifier ``override`` does not override a method
   from a superclass.
-  A method declaration that contains the modifier ``override`` also contains
   the modifiers ``abstract`` or ``static``.


If the signature of the overridden method contains parameters with default
values (see :ref:`Optional Parameters`), then the overriding method always
uses the default parameter values of the overridden method.

A :index:`compile-time error` occurs if a parameter in the overriding method
has a default value.

See :ref:`Overriding by Instance Methods` for the specific rules of overriding.

.. index::
   keyword override
   keyword abstract
   keyword static
   final method
   signature
   overriding
   method
   superclass
   instance
   subclass
   default value
   overridden method
   overriding method

|

.. _Native Methods:

Native Methods
==============

.. meta:
    frontend_status: Done

Native methods are described in the chapter Experimental Features (see
:ref:`Native Methods Experimental`).

|

.. _Methods Overload Signatures:

Method Overload Signatures
==========================

.. meta:
    frontend_status: None
    todo: implement TS overload signature #16181

|LANG| allows specifying a method that has several *overload signatures*,
i.e., several method headers that have the same name followed by one
implementation body.

.. index::
   native method
   method overload
   overload signature
   implementation
   function overload signature
   method overload signature

.. code-block:: abnf

    methodOverloadSignature:
        methodModifier* identifier signature
        ;

A :index:`compile-time error` occurs if the method implementation is not
present, or does not immediately follow the declaration.

A call of a method with overload signatures is always a call of the
implementation method.

The example below has one overload signature parameterless; the other
two have one parameter each:

.. index::
   method implementation
   method declaration
   method overload signature
   overload signature

.. code-block:: typescript
   :linenos:

    class C {
        foo(): void           // 1st signature
        foo(x: string): void  // 2nd signature
        foo(x?: string): void // implementation signature
        {
            console.log(x)
        }
    }
    let c = new C()
    c.foo()          // ok, call fits 1st and 3rd signatures
    c.foo("aa")      // ok, call fits 2nd and 3rd signatures
    c.foo(undefined) // ok, call fits the 3rd signature

The call ``c.foo()`` is executed as a call of the implementation method with
the ``undefined`` argument. The call ``c.foo(x)`` is executed as a call of the
implementation method with an argument.

*Overload signature* compatibility requirements are described in
:ref:`Overload Signature Compatibility`.

In addition, a :index:`compile-time error` occurs if not **all** of the
following requirements are met:

-  Overload signatures and the implementation method have the same access
   modifier.
-  All overload signatures and the implementation method are static or
   non-static.
-  All overload signatures and the implementation method are final or
   non-final.
-  Overload signatures are not native (however, native implementation
   method is allowed).
-  Overload signatures are not abstract.

.. index::
   execution
   call
   signature
   overload signature-compatible
   overload signature
   access modifier
   public
   private
   protected
   abstract
   native implementation method
   final implementation method
   non-final implementation method
   static implementation method
   non-static implementation method

|

.. _Method Body:

Method Body
===========

.. meta:
    frontend_status: Done

A *method body* is a block of code that implements a method. A semicolon, or
an empty body (i.e., no body at all) indicate the absence of the implementation.

An abstract or native method must have an empty body.

In particular, a :index:`compile-time error` occurs if:

-  The body of an abstract or native method declaration is a block.
-  A method declaration is neither abstract nor native, but its body
   is empty, or is a semicolon.


See :ref:`Return Statements` for the rules that apply to return statements
in a method body.

A :index:`compile-time error` occurs if a method is declared to have a return
type, but its body can complete normally (see :ref:`Normal and Abrupt Statement Execution`).

.. index::
   method body
   block
   implementation
   implementation method
   abstract method
   native method
   method declaration
   return statement
   return type
   
|

.. _Inheritance:

Inheritance
===========

.. meta:
    frontend_status: Done

Class *C* inherits from its direct superclass all concrete methods *m* (both
*static* and *instance*) that meet **all** of the following requirements:

-  *m* is a member of the direct superclass of *C*;
-  *m* is public, protected, or internal in the same package as *C*;
-  No signature of a method declared in *C* is compatible with the signature
   of *m* (see :ref:`Compatible Signature`).


Class *C* inherits from its direct superclass and direct superinterfaces all
abstract and default methods *m* (see :ref:`Default Method Declarations`)
that meet the following requirements:

-  *m* is a member of *D*, which is a direct superclass or direct superinterface
   of *C*;
-  *m* is public, protected, or internal in the same package as *C*;
-  No method declared in *C* has a signature that is compatible with the
   signature of *m* (see :ref:`Compatible Signature`);
-  No signature of a concrete method inherited by *C* from its direct
   superclass is compatible with the signature of *m* (see
   :ref:`Compatible Signature`);
-  No method :math:`m'` that is a member of *D*', which is a direct superclass or
   direct superinterface of *C* (while :math:`m'` is distinct from *m*,
   and :math:`D'` from *D*), overrides the declaration of the method *m* from
   :math:`D'` (see :ref:`Overriding by Instance Methods` for class method
   overriding, and :ref:`Overriding by Instance Methods in Interfaces` for
   interface method overriding).


No class can inherit private or static methods from its superinterfaces.

.. index::
   inheritance
   direct superclass
   static method
   instance method
   public
   protected
   package
   signature
   subsignature
   override-equivalent signature
   default method
   abstract method
   direct superinterface
   interface method overriding
   private method
   static method

|

.. _Overriding by Instance Methods:

Overriding by Instance Methods
==============================

.. meta:
    frontend_status: Done

The instance method  *m*:sub:`C` (inherited by, or declared in class *C*)
overrides another method *m*:sub:`A` (declared in class *A*) if **all** the
following requirements are met:

-  *C* is a subclass of *A*;
-  *C* does not inherit *m*:sub:`A`;
-  The signature of *m*:sub:`C` is compatible with the signature of *m*:sub:`A`
   (see :ref:`Compatible Signature`);

---and if one of the following is also true:

-  *m*:sub:`A` is public;
-  *m*:sub:`A` is protected; or
-  *m*:sub:`A` is internal in the same package as *C* while:

    -  Either *C* declares *m*:sub:`C`; or
    -  *m*:sub:`A` is a member of the direct superclass of *C*;

-  *m*:sub:`A` is declared  with package access, and *m*:sub:`C` overrides:

    -  *m*:sub:`A` from a superclass of *C*; or
    -  method :math:`m'` from *C* (while :math:`m'` is distinct from both
       *m*:sub:`C` and *m*:sub:`A`, i.e., :math:`m'` overrides *m*:sub:`A`
       from a superclass of *C*).


.. index::
   instance method
   overriding
   subclass
   inheritance
   signature
   subsignature
   public
   protected
   abstract method
   non-abstract method
   implementation

Non-abstract *m*:sub:`C` implements *m*:sub:`A` from *C* if it overrides an
abstract method *m*:sub:`A`.

An instance method *m*:sub:`C` (inherited by, or declared in class *C*)
overrides another method *m*:sub:`I` (declared in interface *I*) from *C* if
**all** of the following requirements are met:

-  *I* is a superinterface of *C*;
-  *m*:sub:`I` is not static;
-  *C* does not inherit *m*:sub:`I`;
-  The signature of *m*:sub:`C` is a subsignature of the signature of
   *m*:sub:`I` (see :ref:`Override-Equivalent Signatures`); and
-  *m*:sub:`I` is ``public``.


A method call expression (see :ref:`Method Call Expression`) containing the
keyword ``super`` can be used to call an overridden method.

Among the methods that override each other, return types can vary if they are
reference types.

The specialization of return type to a subtype (i.e., *covariant*
:ref:`Covariance` return) is based on the concept of
*return-type-substitutability*. For example, the method declaration *d*:sub:`1`
with return type *R*:sub:`1` is return-type-substitutable for another method
*d*:sub:`2` with return type *R*:sub:`2` if:

-  *R*:sub:`1` is a primitive type (*R*:sub:`2` is identical to *R*:sub:`1`); or
-  *R*:sub:`1` is a reference type (*R*:sub:`1` adapted to type parameters of
   *d*:sub:`2` is a subtype of *R*:sub:`2`).

.. index::
   abstract method
   non-abstract method
   implementation
   overriding
   instance method
   superinterface
   static method
   inheritance
   signature
   subsignature
   keyword super
   qualified name
   overridden method
   superclass type
   return type
   reference type
   return-type-substitutability
   covariant
   covariance
   primitive type
   subtype
   type parameter
  
|

.. _Hiding by Class Methods:

Hiding by Class Methods
=======================

.. meta:
    frontend_status: Done

A static method *m* declared in, or inherited by a class *C* *hides* any
method :math:`m'` (where the signature of *m* is a subsignature of the
signature of :math:`m'` as described in :ref:`Override-Equivalent Signatures`)
in its superclasses and superinterfaces.

A hidden method is not directly accessible (see :ref:`Scopes`) to code in *C*.
However, a hidden method can be accessed by using a qualified name, or a method
call expression (see :ref:`Method Call Expression`) that contains the keyword
``super`` or a cast to a superclass type.

A :index:`compile-time error` occurs if a static method hides an instance
method.

.. index::
   hiding
   static method
   inheritance
   method
   signature
   override-equivalent signature
   superclass
   superinterface
   hidden method
   scope
   access
   qualified name
   method call expression
   keyword super
   superclass type
   instance method
   cast

|

.. _Requirements in Overriding and Hiding:

Requirements in Overriding and Hiding
=====================================

.. meta:
    frontend_status: Done

The method declaration *d*:sub:`1` with return type *R*:sub:`1` can override or
hide the declaration of another method *d*:sub:`2` with return type *R*:sub:`2`
if *d*:sub:`1` is return-type-substitutable for *d*:sub:`2` (see
:ref:`Overriding by Instance Methods`). Otherwise, a compile-time error occurs.

A method that overrides or hides another method (including the methods that
implement abstract methods defined in interfaces) cannot change ``throws`` or
``rethrows`` clauses of the overridden or hidden method.

A compile-time error occurs if a type declaration *T* has a member method
*m*:sub:`1`, but there is also a method *m*:sub:`2` declared in *T* or a
supertype of *T*, for which **all** of the following requirements are met:

-  *m*:sub:`1` and *m*:sub:`2` use the same name; and
-  *m*:sub:`2` is accessible from *T* (see :ref:`Scopes`); and
-  The signature of *m*:sub:`1` is not a subsignature of *m*:sub:`2` (see
   :ref:`Override-Equivalent Signatures`).

.. index::
   overriding
   hiding
   method declaration
   return type
   return-type-substitutability
   abstract method
   interface
   throws clause
   rethrows clause
   hidden method
   overridden method
   compile-time error
   access
   signature
   subsignature
   override-equivalent signature

The access modifier of an overriding or hiding method must provide no less
access than was provided in the overridden or hidden method.

A compile-time error occurs if:

-  The overridden or hidden method is public, and the overriding or hiding
   method is *not* public.
-  The overridden or hidden method is protected, and the overriding or hiding
   method is *not* protected or public.
-  The overridden or hidden method has internal access, and the overriding
   or hiding method is private.

.. index::
   overriding method
   hiding method
   access modifier
   overridden method
   compile-time error
   hidden method
   public method
   protected method
   private method
   internal access

|

.. _Inheriting Methods with Override-Equivalent Signatures:

Inheriting Methods with Override-Equivalent Signatures
======================================================

.. meta:
    frontend_status: Done

A class can inherit multiple methods with override-equivalent signatures (see
:ref:`Override-Equivalent Signatures`).

A compile-time error occurs if a class *C* inherits the following:

-  Concrete method whose signature is override-equivalent to another
   method that *C* inherited; or
-  Default method whose signature is override-equivalent to another method
   that *C* inherited, if there is no abstract method, declared in a
   superclass of *C* and inherited by *C*, that is override-equivalent
   to both methods.


An abstract class can inherit all the methods, assuming that a set of
override-equivalent methods consists of at least one abstract method, and
zero or more default methods.

A compile-time error occurs if one of the inherited methods is not
return-type-substitutable for every other inherited method (except ``throws``
and ``rethrows`` clauses that cause no error in this case).

The same method declaration can be inherited from an interface in a number
of ways, causing no compile-time error on its own.

.. index::
   inheriting method
   override-equivalent signature
   inheritance
   compile-time error
   abstract method
   superclass
   return-type-substitutability
   inherited method
   throws clause
   rethrows clause
   interface
   method declaration

|

.. _Accessor Declarations:

Accessor Declarations
*********************

.. meta:
    frontend_status: Done

Accessors are often used instead of fields to add additional control for
operations of getting or setting a field value. An accessor can be either
a getter or a setter.

.. code-block:: abnf

    classAccessorDeclaration:
        accessorModifier
        ( 'get' identifier '(' ')' returnType block?
        | 'set' identifier '(' parameter ')' block?
        )
        ;

    accessorModifier:
        'abstract'
        | 'static'
        | 'final'
        | 'override'
        ;

Accessor modifiers are a subset of method modifiers. The allowed accessor
modifiers have exactly the same meaning as the corresponding method modifiers.
See :ref:`Abstract Methods` for the modifier ``abstract``,
:ref:`Static Methods` for the modifier ``static``, :ref:`Final Methods`
for the modifier ``final``, and :ref:`Override Methods` for the modifier
``override``.

.. index::
   access declaration
   field
   field value
   accessor
   getting
   setting
   getter
   setter
   expression
   accessor modifier
   method modifier
   abstract
   static method
   final method
   override method

.. code-block:: typescript
   :linenos:

    class Person {
      private _age: number = 0
      get age(): number { return this._age }
      set age(a: number) {
        if (a < 0) { throw new Error("wrong age") }
        this._age = a
      }
    }

Each *get-accessor* (*getter*) must have neither parameters nor an explicit
return type. Each *set-accessor* (*setter*) must have a single parameter and
no return value. The use of getters and setters looks the same as the use of
fields:

.. code-block:: typescript
   :linenos:

    class Person {
      private _age: number = 0
      get age(): number { return this._age }
      set age(a: number) {
        if (a < 0) { throw new Error("wrong age") }
        this._age = a
      }
    }

    let p = new Person()
    p.age = 25        // setter is called
    if (p.age > 30) { // getter is called
      // do something
    }

A class can define a getter, a setter, or both with the same name.
If both a getter and a setter with a particular name are defined,
then both must have the same accessor modifiers. Otherwise, a compile-time
error occurs.

Accessors can be implemented by using a private field to store its value
(as in the example above).

.. index::
   accessor
   getter
   setter
   explicit return type
   return value
   parameter
   private field
   class
   compile-time error
   accessor modifier

.. code-block:: typescript
   :linenos:

    class Person {
      name: string = ""
      surname: string = ""
      get fullName(): string {
        return this.surname + " " + this.name
      }
    }

|

.. _Class Initializer:

Class Initializer
*****************

.. meta:
    frontend_status: Done

When a class is initialized, the *class initializer* declared in the class is
executed along with all *class initializers* of all superclasses. The order of
execution is from the top superclass to the current class. Class initializers
(along with field initializers for static fields as described in
:ref:`Field Initialization`) ensure that all static fields receive their
initial values before the first use.

.. code-block:: typescript
   :linenos:

    classInitializer
        : 'static' block
        ;

A compile-time error occurs if a class initializer contains:

-  A ``return <expression>`` statement (see :ref:`Return Statements`).
-  A ``throw`` statement (see :ref:`Throw Statements`) with no surrounding
   ``try`` statement (see :ref:`Try Statements`) to handle the error or exception.
-  Keywords ``this`` (see :ref:`this Expression`) or ``super`` (see
   :ref:`Method Call Expression` and :ref:`Field Access Expressions`), or any
   type of a variable declared outside the class initializer.


Restrictions of class initializers’ ability to refer to static fields (even
those within the scope) are specified in :ref:`Exceptions and Errors Inside Field Initializers`.
Class initializers cannot throw exceptions as they are effectively
non-throwing functions (see :ref:`Non-Throwing Functions`).

.. index::
   class initializer
   execution
   static field
   field initialization
   initial value
   compile-time error
   return expression statement
   throw statement
   try statement
   keyword this
   keyword super
   method call
   field access
   restriction
   scope
   exception
   error
   non-throwing function

|

.. _Constructor Declaration:

Constructor Declaration
***********************

.. meta:
    frontend_status: Done
    todo: Explicit Constructor Call - "Qualified superclass constructor calls" - not implemented, need more investigation (inner class)

*Constructors* are used to initialize objects that are instances of class.

A *constructor declaration* starts with the keyword ``constructor``, and has no
name. In any other syntactical aspect, a constructor declaration is similar to
a method declaration with no return type.

.. code-block:: abnf

    constructorDeclaration:
        constructorOverloadSignature*
        'constructor' '(' parameterList? ')' throwMark? constructorBody
        ;

    throwMark:
        'throws'
        | 'rethrows'
        ;

Constructors are called by the following:

-  Class instance creation expressions (see :ref:`New Expressions`);
-  Conversions and concatenations caused by the string concatenation operator
   '``+``' (see :ref:`String Concatenation`); and
-  Explicit constructor calls from other constructors (see :ref:`Constructor Body`).

Access to constructors is governed by access modifiers (see
:ref:`Access Modifiers` and :ref:`Scopes`). Declaring a constructor
inaccessible prevents class instantiation from using this constructor.
If the only constructor is declared inaccessible, then no class instance
can be created.

A compile-time error occurs if two constructors in a class are declared, and
have identical signatures.

See :ref:`Throwing Functions` for the ``throws``, and :ref:`Rethrowing Functions`
for the ``rethrows`` mark.

.. index::
   constructor
   constructor declaration
   object
   creation
   instance
   instance creation
   instance creation expression
   expression
   class
   keyword constructor
   class instance
   concatenation
   conversion
   string concatenation operator
   explicit constructor call
   throwing function
   rethrowing function
   throws mark
   rethrows mark
   scope
   compile-time error
   access modifier
   access
   class instantiation
   signature

|

.. _Formal Parameters:

Formal Parameters
=================

.. meta:
    frontend_status: Done

The syntax and semantics of a constructor’s formal parameters are identical
to those of a method.

.. _Constructors Overload Signatures:

Constructor Overload Signatures
===============================

.. meta:
    frontend_status: None
    todo: implement TS overload signature #16181

|LANG| allows specifying a constructor that can be called in different ways by
providing *overload signatures*, i.e., several constructor headers which are
followed by one constructor implementation body.

.. index::
   overload signature
   constructor overload signature

.. code-block:: abnf

    constructorOverloadSignature:
        accessModifier? 'constructor' signature
        ;

A :index:`compile-time error` occurs if the constructor implementation is not
present, or does not immediately follow the declaration.

A call of a constructor with overload signature is always a call of the
constructor implementation body.

The example below has one overload signature parameterless, and others have one
parameter each:

.. code-block:: typescript
   :linenos:

    class C {
        constructor()           // 1st signature
        constructor(x: string)  // 2nd signature
        constructor(x?: string) // 3rd - implementation signature
        {
            console.log(x)
        }
    }
    new C()          // ok, fits the 1st and 3rd signatures
    new C("aa")      // ok, fits the 2nd and 3rd signatures
    new C(undefined) // ok, fits 3rd signature

The new expression (see :ref:`New Expressions`) ``new C()`` leads to a call of
the constructor implementation with the argument  ``undefined``. The ``new C(x)``
creates an object calling constructor implementation with 'x' as an argument.

*Overload signature* compatibility requirements are described in
:ref:`Overload Signature Compatibility`.

A :index:`compile-time error` occurs if at least two different overload
signatures or implementation signatures have different *access modifiers*.

.. code-block:: typescript
   :linenos:

    class Incorrect {
        // Constructors have different access modifiers
        private constructor()             // private 1st signature
        protected constructor(x: string)  // protected 2nd signature
        constructor(x?: string)           // public 3rd - implementation signature
        {}
    }


|

.. _Constructor Body:

Constructor Body
================

.. meta:
    frontend_status: Done

The constructor body must provide correct initialization of new class instances.
Constructors have two variations:

- *Primiry constructor* that initializes its instance [1]_ own fields directly;

- *Secondary constuctor* that uses another same-class constructor to initialize
  its instance fields.

The syntax of both variations is the same:

.. code-block:: abnf

    constructorBody:
        '{' statement* constructorCall? statement* '}'
        ;

    constructorCall:
        'this' arguments
        | 'super' arguments
        ;


The high-level sequence of a *primary constructor* body includes the following:

1. Optional arbitrary code that does not use ``this`` or ``super``.

2. Mandatory call to ``super([arguments])`` (see :ref:`Explicit Constructor Call`)
   if a class has an extension clause (see :ref:`Class Extension Clause`).
   
3. Implicitly added compiler-generated code that:

    - Sets default values for instance own fields.
    
    - Executes instance own field initializers in a valid order determined by
      the compiler. If the compiler detects a circular reference, then a
      compile-time error occurs.

4. Arbitary code that guarantees all remaining instance fields (if any) are
   initialized but does not:

    - Use the value of an instance field before its initialization.
    - Call any instance method before all instance own fields are initialized.

5. Optional arbitrary code.

The example below shows *primary constuctors*:

.. code-block:: typescript
   :linenos:

    class Point {
      x: number
      y: number
      constructor(x: number, y: number) {
        this.x = x
        this.y = y
      }
    }

    class ColoredPoint extends Point {
      static readonly WHITE = 0
      static readonly BLACK = 1
      color: number
      constructor(x: number, y: number, color: number) {
        super(x, y) // calls base class constructor
        this.color = color
      }
    }

.. index::
   statement
   constructor body
   constructor call
   direct superclass


The high-level sequence of a *secondary constructor* body includes the following:

1. Optional arbitrary code that does not use ``this`` or ``super``.

2. Call to another same-class constructor ``this([arguments])`` with arguments.

3. Optional arbitrary code.

The example below shows *primary* and *secondary* constuctors:

.. code-block:: typescript
   :linenos:

    class ColoredPoint extends Point {
      static readonly WHITE = 0
      static readonly BLACK = 1
      color: number

      // primary constructor:   
      constructor(x: number, y: number, color: number) {
        super(x, y) // calls base class constructor as class has 'extends'
        this.color = color
      }
      // secondary constructor: 
      constructor(color: number) {
        this(0, 0, color) 
      }
    }


A compile-time error occurs if a constructor calls itself, directly or
indirectly, through a series of one or more explicit constructor calls
using ``this``.

A constructor body looks like a method body (see :ref:`Method Body`), except
the semantics described above, and explicit return of a value
(see :ref:`Return Statements`) is prohibited. However, a return statement
without an expression can be used in a constructor body.

A constructor body must not use fields of a created object before the fields
are initialized; ``this`` cannot be passed as an argument until each object
field receives an initial value. These checks are performed by the compiler
that reports a compile-time error if a violation is detected. 

.. index::
   compile-time error
   constructor call
   constructor body
   superclass
   direct superclass
   argument
   primordial class
   Object
   method body
   return statement
   expression
   this
   super()

|

.. _Explicit Constructor Call:

Explicit Constructor Call
=========================

.. meta:
    frontend_status: Done

There are two kinds of *explicit constructor call* statements:

-  *Alternate constructor calls* that begin with the keyword ``this``, and
   can be prefixed with explicit type arguments (used to call an alternate
   same-class constructor).
-  *Superclass constructor calls* (used to call a constructor from
   the direct superclass) called *unqualified superclass constructor calls*
   that begin with the keyword ``super``, and can be prefixed with explicit
   type arguments.


A compile-time error occurs if the constructor body of an explicit
constructor call statement:

-  Refers to any non-static field or instance method; or
-  Uses the keywords ``this`` or ``super`` in any expression.

.. index::
   constructor call
   constructor call statement
   alternate constructor call
   keyword this
   superclass constructor call
   direct superclass constructor
   unqualified superclass constructor call statement
   keyword super
   prefix
   explicit type argument
   compile-time error
   constructor body
   non-static field
   instance method
   superclass
   expression
   instantiation
   enclosing
   qualified superclass constructor call statement
   static context
   

An ordinary method call evaluates an alternate constructor call statement
left-to-right. The evaluation starts from arguments, proceeds to constructor,
and then the constructor is called.

The process of evaluation of a superclass constructor call statement is
performed as follows:

.. index::
   expression
   qualified superclass constructor call statement
   subclass
   access
   scope
   method call
   evaluation
   alternate constructor call statement
   argument
   constructor
   superclass constructor call statement

1. If instance *i* is created, then the following procedure is used to
   determine *i*'s immediately enclosing instance with respect to *S*
   (if available):

   -  If the declaration of *S* occurs in a static context, then *i* has no
      immediately enclosing instance with respect to *S*.

   -  If the superclass constructor call is unqualified, then *S* must be a
      local class.

      If *S* is a local class, then the immediately enclosing type declaration
      of *S* is *O*.

      If *n* is an integer (:math:`n\geq{}1`), and *O* is the *n*’th
      lexically enclosing type declaration of *C*, then *i*'s immediately
      enclosing instance with respect to *S* is the *n*’th lexically enclosing
      instance of ``this``.

.. index::
   instance
   creation
   enclosing instance
   static context
   superclass constructor call
   qualified superclass constructor call
   unqualified superclass constructor call
   enclosing type declaration
   integer
   lexically enclosing type declaration
   lexically enclosing instance
   expression
   evaluation

2. After *i*'s immediately enclosing instance with respect to *S* (if available)
   is determined, the evaluation of the superclass constructor call statement
   continues left-to-right. The arguments to the constructor are evaluated, and
   then the constructor is called.

3. If the superclass constructor call statement completes normally after all,
   then all non-static field initializers of *C* are executed. *I* is executed
   before *J* if a non-static field initializer *I* textually precedes another
   non-static field initializer *J*.


   Non-static field initializers are executed if the superclass constructor
   call:
   
   -  Has an explicit constructor call statement; or
   -  Is implicit.


   An alternate constructor call does not perform the implicit execution.

.. index::
   immediately enclosing instance
   evaluation
   superclass constructor call
   superclass constructor call statement
   argument
   constructor
   non-static field initializer
   execution
   alternate constructor call statement

|

.. _Default Constructor:

Default Constructor
===================

.. meta:
    frontend_status: Done

If a class contains no constructor declaration, then a default constructor
is implicitly declared. This guarantees that every class has effectively at
least one constructor. The default constructor has the following form:

-  The access modifier of the default constructor is ``public`` (see :ref:`Access Modifiers`).

-  The default constructor has no ``throws`` or ``rethrows`` clauses.

-  The body of the default constructor contains a call to the superclass
   constructor with no arguments except the primordial class ``Object``. The
   body of the default constructor for the primordial class ``Object`` is empty.

A compile-time error occurs if a default constructor is implicit, but
the superclass:

-  Has no accessible constructor without parameters; and
-  Has a constructor without parameters but with ``throws`` or ``rethrows``
   clauses.

.. code-block:: typescript
   :linenos:
    
   // Class declarations without constructors
   class Object {}
   class Base {}
   class Derived extends Base {}


   // Class declarations with default constructors declared implicitly
   class Object {
     constructor () {} // Empty body - as there is no supercalss
   }
   // Default constructors added
   class Base { constructor () { super () } }
   class Derived extends Base { constructor () { super () } }

   // Example of an error case #1
   class A {
       private constructor () {}
   }
   class B extends A {} // No constructor in B
   // During compilation of B
   class B extends A { constructor () { super () } } // Default constructor added
   // And it leads to compile-time error as default constructor calls super()
   // which is private and inaccessible

   // Example of an error case #2
   class A {
       constructor () throws {}
   }
   class B extends A {} // No constructor in B
   // During compilation of B
   class B extends A { constructor () { super () } } // Default constructor added
   // And it leads to compile-time error as default constructor is not marked as throws
   // but it call super() which throws


.. index::
   default constructor
   constructor declaration
   field
   default value
   top-level class
   local class
   access modifier
   internal access
   throws clause
   rethrows clause
   primordial class
   Object
   compile-time error
   accessible constructor

|

.. _Local Classes and Interfaces:

Local Classes and Interfaces
****************************

.. meta:
    frontend_status: Done

Local classes and interfaces (see :ref:`Interfaces`) are declared within the
body of a function, method, or any block delimited by balanced braces in a
group of zero or more statements.

Names of local classes and interfaces are visible only within the scope they
are declared in. When declared in a scope, names of local classes and
interfaces have access to entities visible within this scope, and capture the
entities they use from this scope. Function/method parameters and local
variables can be used and thus captured.

A compile-time error occurs if:

-  A local class or interface declaration has access modifier ``public``,
   ``protected``, or ``private``.
-  A local class or interface declaration members have access modifier
   ``public``, ``protected``, ``private``, or ``export``.


The example below shows local classes and interfaces in a top-level function:

.. code-block:: typescript
   :linenos:
    
    function foo (parameter: number) {
      let local: string = "function local"
      interface LocalInterface { // Local interface in a top-level function
        method (): void // It has a method
        field: string   // and a property
      }
      class LocalClass implements LocalInterface { // Class implements interface
        // Local class in a top-level function
        method () { console.log ("Instance field = ", this.field, " par = ", parameter, " loc = ", local) }
        field: string = "`instance field value`"
        static method () { console.log ("Static field = ", LocalClass.field) }
        static field: string = "`class/static field value`"
      }
      let lc: LocalInterface  = new LocalClass 
        // Both local types can be freely used in the top-level function scope
      lc.method()
      LocalClass.method()
    }


The example below shows local classes and interfaces in a class method. The
algorithm is similar to that in a top-level function. However, the
surrounding class members are not accessible from local classes:

.. code-block:: typescript
   :linenos:

    class A_class {
      field: number = 1234 // Not visible for the local class
      method (parameter: number) {
        let local: string = "instance local"
        interface LocalInterface {
           method (): void
           field: string
        }
        class LocalClass implements LocalInterface {
           method () { console.log ("Instance field = ", this.field, " par = ", parameter, " loc = ", local) }
           field: string = "`instance field value`"
           static method () { console.log ("Static field = ", LocalClass.field) }
           static field: string = "`class/static field value`"
        }
        let lc: LocalInterface  = new LocalClass
        lc.method()
        LocalClass.method()
      }
      static method (parameter: number) {
        let local: string = "class/static local"
        interface LocalInterface {
           method (): void
           field: string
        }
        class LocalClass implements LocalInterface {
           method () { console.log ("Instance field = ", this.field, " par = ", parameter, " loc = ", local) }
           field: string = "`instance field value`"
           static method () { console.log ("Static field = ", LocalClass.field) }
           static field: string = "`class/static field value`"
        }
        let lc: LocalInterface  = new LocalClass
        lc.method()
        LocalClass.method()
      }
    }


-------------

.. [1]
   Class own fields here means fields declared in the class.

.. raw:: pdf

   PageBreak


