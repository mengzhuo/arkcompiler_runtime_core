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

.. _Lexical Elements:

Lexical Elements
################

.. meta:
    frontend_status: Done

This chapter discusses the lexical structure of the |LANG| programming language
and the analytical conventions.

|

.. _Unicode Characters:

Use of Unicode Characters
*************************

.. meta:
    frontend_status: Done

The |LANG| programming language uses characters of the Unicode Character
set [1]_ as its terminal symbols. It represents text in sequences of
16-bit code units using the Unicode UTF-16 encoding.

The term *Unicode code point* is used in this specification only where such
representation is relevant to refer the reader to Unicode Character set and
UTF-16 encoding. Where such representation is irrelevant to the discussion,
the generic term *character* is used.

.. index::
   terminal symbol
   character
   Unicode character
   Unicode code point

|

.. _Lexical Input Elements:

Lexical Input Elements
**********************

.. meta:
    frontend_status: Done

The language has lexical input elements of the following types:

-  :ref:`White Spaces`,
-  :ref:`Line Separators`,
-  :ref:`Tokens`, and
-  :ref:`Comments`.

|

.. _White Spaces:

White Spaces
************

.. meta:
    frontend_status: Done

White spaces are lexical input elements that separate tokens from one another.
Whitespaces never occur within a token. White spaces include the following:

- Space (U+0020),

- Horizontal tabulation (U+0009),

- Vertical tabulation (U+000B),

- Form feed (U+000C),

- No-break space (U+00A0), and

- Zero-width no-break space (U+FEFF).


White spaces improve source code readability and help avoiding ambiguities.
White spaces are ignored by the syntactic grammar, but can occur within a comment.

.. index::
   lexical input element
   source code
   white space
   syntactic grammar
   comment

|

.. _Line Separators:

Line Separators
***************

.. meta:
    frontend_status: Done

Line separators are lexical input elements that divide sequences of Unicode
input characters into lines. Line separators include the following:

- Newline character (U+000A or ASCII <LF>),

- Carriage return character (U+000D or ASCII <CR>),

- Line separator character (U+2028 or ASCII <LS>), and

- Paragraph separator character (U+2029 or ASCII <PS>).

Line separators separate tokens from one another and improve source code
readability. Any sequence of line separators is considered a single separator.

|

.. _Tokens:

Tokens
******

.. meta:
    frontend_status: Done

Tokens form the vocabulary of the language. There are four classes of tokens:

-  :ref:`Identifiers`,
-  :ref:`Keywords`,
-  :ref:`Operators and Punctuators`, and
-  :ref:`Literals`.


Token is the only lexical input element that can act as a terminal symbol
of the syntactic grammar. In the process of tokenization, the next token is
always the longest sequence of characters that form a valid token. Tokens
are separated by white spaces (see :ref:`White spaces`). Without white spaces,
tokens merge into a single token. White spaces are ignored by the syntactic
grammar.

Line separators are often treated as white spaces, except where line
separators have special meanings. See :ref:`Semicolons` for more details.

.. index::
   line separator
   lexical input element
   Unicode input character
   token
   tokenization
   white space
   source code
   identifier
   keyword
   operator
   punctuator
   literal
   terminal symbol
   syntactic grammar

|

.. _Identifiers:

Identifiers
***********

.. meta:
    frontend_status: Done

An identifier is a sequence of one or more valid Unicode characters. The
Unicode grammar of identifiers is based on character properties
specified by the Unicode Standard.

The first character in an identifier must be '``$``', '``_``', or any Unicode
code point with the Unicode property 'ID_Start'[2]_. Other characters
must be Unicode code points with the Unicode property, or one of the following
characters:

-  '``$``' (\\U+0024),
-  'Zero-Width Non-Joiner' (<ZWNJ>, \\U+200C), or
-  'Zero-Width Joiner' (<ZWNJ>, \\U+200D).

.. index::
   identifier
   Unicode Standard
   identifier
   Unicode code point
   Unicode character
   
.. code-block:: abnf

    Identifier:
      IdentifierStart IdentifierPart \*
      ;

    IdentifierStart:
      UnicodeIDStart
      | '$'
      | '_'
      | '\\' EscapeSequence
      ;

    IdentifierPart:
      UnicodeIDContinue
      | '$'
      | <ZWNJ>
      | <ZWJ>
      | '\\' EscapeSequence
      ;

|

.. _Keywords:

Keywords
********

.. meta:
    frontend_status: Done

*Keywords* are the reserved words that have permanently predefined meanings
in |LANG|. Keywords are always lowercase. Keywords can be of four kinds as
discussed below.

1. The following keywords are reserved in any context (*hard keywords*), and
cannot be used as identifiers:

.. index::
   keyword
   reserved word
   hard keyword
   soft keyword
   identifier
   context
   
+--------------------+-------------------+------------------+------------------+
|                    |                   |                  |                  |
+====================+===================+==================+==================+
|   ``abstract``     |   ``else``        |   ``internal``   |    ``static``    |
+--------------------+-------------------+------------------+------------------+
|   ``as``           |   ``enum``        |   ``launch``     |    ``switch``    |
+--------------------+-------------------+------------------+------------------+
|   ``assert``       |   ``export``      |   ``let``        |    ``super``     |
+--------------------+-------------------+------------------+------------------+
|   ``async``        |   ``extends``     |   ``native``     |    ``this``      |
+--------------------+-------------------+------------------+------------------+
|   ``await``        |   ``false``       |   ``new``        |    ``throw``     |
+--------------------+-------------------+------------------+------------------+
|   ``break``        |   ``final``       |   ``null``       |    ``true``      |
+--------------------+-------------------+------------------+------------------+
|   ``case``         |   ``for``         |   ``override``   |    ``try``       |
+--------------------+-------------------+------------------+------------------+
|   ``class``        |   ``function``    |   ``package``    |    ``undefined`` |
+--------------------+-------------------+------------------+------------------+
|   ``const``        |   ``if``          |   ``private``    |    ``while``     |
+--------------------+-------------------+------------------+------------------+
|   ``constructor``  |   ``implements``  |   ``protected``  |                  |
+--------------------+-------------------+------------------+------------------+
|   ``continue``     |   ``import``      |   ``public``     |                  |
+--------------------+-------------------+------------------+------------------+
|   ``do``           |   ``interface``   |   ``return``     |                  |
+--------------------+-------------------+------------------+------------------+

2. The following words have special meaning in certain contexts (*soft
keywords*) but are valid identifiers elsewhere:

.. index::
   keyword
   soft keyword
   identifier

+-----------------+--------------------+-------------------+-------------------+
|                 |                    |                   |                   |
+=================+====================+===================+===================+
|    ``catch``    |    ``get``         |    ``out``        |    ``throws``     |
+-----------------+--------------------+-------------------+-------------------+
|    ``declare``  |    ``in``          |    ``readonly``   |    ``type``       |
+-----------------+--------------------+-------------------+-------------------+
|    ``default``  |   ``instanceof``   |    ``rethrows``   |    ``typeof``     |
+-----------------+--------------------+-------------------+-------------------+
|    ``finally``  |    ``keyof``       |    ``set``        |                   |
+-----------------+--------------------+-------------------+-------------------+
|    ``from``     |    ``of``          |    ``struct``     |                   |
+-----------------+--------------------+-------------------+-------------------+


3. The following words cannot be used as user-defined type names but are
not otherwise restricted:

.. index::
   user-defined type name

+---------------+---------------+---------------+
|               |               |               |
+===============+===============+===============+
| ``boolean``   | ``double``    | ``number``    |
+---------------+---------------+---------------+
| ``byte``      | ``float``     | ``short``     |
+---------------+---------------+---------------+
| ``bigint``    | ``int``       | ``string``    |
+---------------+---------------+---------------+
| ``char``      | ``long``      | ``void``      |
+---------------+---------------+---------------+

4. The following identifiers are also treated as *soft keywords* reserved for
the future use (or used in TS):

.. index::
   identifier
   keyword

+-------------------------+-------------------------+-------------------------+
|                         |                         |                         |
+=========================+=========================+=========================+
|        ``is``           |        ``var``          |        ``yield``        |
+-------------------------+-------------------------+-------------------------+


|

.. _Operators and Punctuators:

Operators and Punctuators
*************************

.. meta:
    frontend_status: Done

*Operators* are tokens that denote various actions to be performed on values:
addition, subtraction, comparison, and other. The keywords ``instanceof`` and
``typeof`` also act as operators.

*Punctuators* are tokens that separate, complete, or otherwise organize program
elements and parts: commas, semicolons, parentheses, square brackets, etc.

The following character sequences represent operators and punctuators:

.. index::
   operator
   token
   value
   addition
   subtraction
   comparison
   punctuator

+-------+--------+--------+----------+--------+---------+---------+-------+-------+
+-------+--------+--------+----------+--------+---------+---------+-------+-------+
|       |        |        | ``&=``   |        | ``==``  | ``??``  |       |       |
+-------+--------+--------+----------+--------+---------+---------+-------+-------+
| ``+`` | ``&``  | ``+=`` | ``|=``   |        | ``<``   | ``?.``  | ``(`` | ``)`` |
+-------+--------+--------+----------+--------+---------+---------+-------+-------+
| ``-`` | ``|``  | ``-=`` | ``^=``   | ``&&`` | ``>``   | ``!.``  | ``[`` | ``]`` |
+-------+--------+--------+----------+--------+---------+---------+-------+-------+
| ``*`` | ``^``  | ``*=`` | ``<<=``  | ``||`` | ``===`` | ``<=``  | ``{`` | ``}`` |
+-------+--------+--------+----------+--------+---------+---------+-------+-------+
| ``/`` | ``>>`` | ``/=`` | ``>>=``  | ``++`` | ``=``   | ``>=``  | ``,`` | ``;`` |
+-------+--------+--------+----------+--------+---------+---------+-------+-------+
| ``%`` | ``<<`` | ``%=`` | ``>>>=`` | ``--`` | ``!``   | ``...`` | ``.`` | ``:`` |
+-------+--------+--------+----------+--------+---------+---------+-------+-------+

|

.. _Literals:

Literals
********

.. meta:
    frontend_status: Done

*Literals* are representations of certain value types.

.. code-block:: abnf

    Literal:
      IntegerLiteral
      | FloatLiteral
      | BigIntLiteral
      | BooleanLiteral
      | StringLiteral
      | TemplateLiteral
      | NullLiteral
      | UndefinedLiteral
      | CharLiteral
      ;

See :ref:`Character Literals` for the experimental ``char literal``.

.. index::
   literal
   value type
   char

|
   
.. _Integer Literals:

Integer Literals
================

.. meta:
    frontend_status: Done

Integer literals represent numbers that do not have a decimal point or
an exponential part. Integer literals can be written with bases 16
(hexadecimal), 10 (decimal), 8 (octal), and 2 (binary).

.. index::
   integer
   literal
   hexadecimal
   decimal
   octal
   binary
   
   
.. code-block:: abnf

    IntegerLiteral:
      DecimalIntegerLiteral
      | HexIntegerLiteral
      | OctalIntegerLiteral
      | BinaryIntegerLiteral
      ;

    DecimalIntegerLiteral:
      '0'
      | [1-9] ('_'? [0-9])* 
      ;

    HexIntegerLiteral:
      '0' [xX]  ( HexDigit
      | HexDigit (HexDigit | '_')* HexDigit
      )
      ;

    HexDigit:
      [0-9a-fA-F]
      ;

    OctalIntegerLiteral:
      '0' [oO] ( [0-7] | [0-7] [0-7_]* [0-7] )
      ;

    BinaryIntegerLiteral:
      '0' [bB] ( [01] | [01] [01_]* [01] )
      ;

It is presented by the examples below:

.. code-block:: typescript
   :linenos:

    153 // decimal literal
    1_153 // decimal literal
    0xBAD3 // hex literal
    0xBAD_3 // hex literal
    0o777 // octal literal
    0b101 // binary literal

The underscore character '``_``' after a base prefix or between successive
digits can be used to denote an integer literal and improve readability.
Underscore characters in such positions do not change the values of literals.
However, an underscore character must not be the very first or the very last
symbol of an integer literal.

.. index::
   prefix
   value
   literal
   integer
   underscore character

Integer literals are of type ``int`` if the value can be represented by a
32-bit number; it is of type ``long`` otherwise. In variable and constant
declarations, an integer literal can be implicitly converted to another
integer type or type ``char`` (see :ref:`Type Compatibility with Initializer`).
In all other places an explicit cast must be used (see :ref:`Cast Expressions`).

.. index::
   integer literal
   int
   long
   constant declaration
   variable declaration
   integer literal
   char
   explicit cast
   implicit conversion
   cast expression

|

.. _Floating-Point Literals:

Floating-Point Literals
=======================

.. meta:
    frontend_status: Done

*Floating-point literals* represent decimal numbers and consist of a
whole-number part, a decimal point, a fraction part, an exponent, and
a float type suffix:

.. code-block:: abnf

    FloatLiteral:
        DecimalIntegerLiteral '.' FractionalPart? ExponentPart? FloatTypeSuffix?
        | '.' FractionalPart ExponentPart? FloatTypeSuffix?
        | DecimalIntegerLiteral ExponentPart FloatTypeSuffix?
        ;

    ExponentPart:
        [eE] [+-]? DecimalIntegerLiteral
        ;

    FractionalPart:
        [0-9]
        | [0-9] [0-9_]* [0-9]
        ;
    FloatTypeSuffix:
        'f'
        ;

It is presented by the examples below:

.. code-block:: typescript
   :linenos:

    3.14
    3.14f
    3.141_592
    .5
    1e10
    1e10f

The underscore character '``_``' after a base prefix or between successive
digits can be used to denote a floating-point literal and improve readability.
Underscore characters in such positions do not change the values of literals.
However, an underscore character must not be the very first and the very
last symbol of an integer literal.

A floating-point literal is of type ``float`` if *float type suffix* is present.
Otherwise, it is of type ``double`` (type ``number`` is an alias to ``double``).

A compile-time error occurs if a non-zero floating-point literal is
too large for its type.

A floating-point literal in variable and constant declarations can be implicitly
converted to type ``float`` (see :ref:`Type Compatibility with Initializer`).

.. index::
   floating-point literal
   compile-time error
   prefix
   underscore character
   implicit conversion
   constant declaration

|

.. _BigInt Literals:

``BigInt`` Literals
===================

.. meta:
    frontend_status: Done

``BigInt`` literals represent integer numbers with unlimited number of digits.
``BigInt`` literals use decimal base only. A ``BigInt`` literal is a sequence
of digits followed by the symbol '``n``':

.. code-block:: abnf

    BigIntLiteral:
      '0n'
      | [1-9] ('_'? [0-9])* 'n'
      ;

It is presented by the examples below:

.. code-block:: typescript

    153n // bigint literal
    1_153n // bigint literal

The underscore character '``_``' used between successive digits can be used to
denote a ``BigInt`` literal and improve readability. Underscore characters in
such positions do not change the values of literals. However, an underscore
character must not be the very first or the very last symbol of a ``BigInt``
literal.

``BigInt`` literals are always of type ``bigint``.

Strings that represent numbers or any integer values can be converted to
``bigint`` by using the built-in functions:

.. code-block:: typescript

    BigInt (other: string): bigint
    BigInt (other: long): bigint

.. index::
   integer
   BigInt literal
   underscore character

Two other static methods allow taking *bitsCount* lower bits of a
``BigInt`` number and return them as a result. Signed and unsigned versions
are both possible:

.. code-block:: typescript

    BigInt.asIntN(bitsCount: long, bigIntToCut: bigint): bigint
    BigInt.asUintN(bitsCount: long, bigIntToCut: bigint): bigint

.. index::
   static method


.. _Boolean Literals:

``Boolean`` Literals
====================

.. meta:
    frontend_status: Done

The two ``Boolean`` literal values are represented by the keywords
``true`` and ``false``.

.. code-block:: abnf
   :linenos:

    BooleanLiteral:
        ’true’ | ’false’
        ;

``Boolean`` literals are of type ``boolean``.

.. index::
   keyword
   Boolean literal

|

.. _String Literals:

``String`` Literals
===================

.. meta:
    frontend_status: Done
    todo: "" sample is invalid: SyntaxError: Newline is not allowed in strings

``String`` literals consist of zero or more characters enclosed between
single or double quotes. A special form of string literals is
*template literal* (see :ref:`Template Literals`).

``String`` literals are of type ``string``. Type ``string`` is a predefined
reference type (see :ref:`Type String`).

.. index::
   string literal
   template literal
   predefined reference type


.. code-block:: abnf

    StringLiteral:
        '"' DoubleQuoteCharacter* '"'
        | '\'' SingleQuoteCharacter* '\''
        ;

    DoubleQuoteCharacter:
        ~["\\\r\n]
        | '\\' EscapeSequence
        ;

    SingleQuoteCharacter:
        ~['\\\r\n]
        | '\\' EscapeSequence
        ;

    EscapeSequence:
        ['"bfnrtv0\\]
        | 'x' HexDigit HexDigit
        | 'u' HexDigit HexDigit HexDigit HexDigit
        | 'u' '{' HexDigit+ '}'
        | ~[1-9xu\r\n]
        ;

Normally, characters in ``string`` literals represent themselves. However,
certain non-graphic characters can be represented by explicit specifications
or Unicode codes. Such constructs are called *escape sequences*.

Escape sequences can represent graphic characters within a ``string`` literal,
e.g., single quotes '``’``', double quotes '``”``', backslashes '``\``', and
some others.

.. index::
   string literal
   escape sequence
   backslash
   single quote
   double quotes

An escape sequence always starts with the backslash character '``\``', followed
by one of the following characters:

-  ``”`` (double quote, U+0022),

.. "

-  ``'`` (neutral single quote, U+0027),

.. ’ U+2019

-  ``b`` (backspace, U+0008),

-  ``f`` (form feed, U+000c),

-  ``n`` (linefeed, U+000a),

-  ``r`` (carriage return, U+000d),

-  ``t`` (horizontal tab, U+0009),

-  ``v`` (vertical tab, U+000b),

-  ``\`` (backslash, U+005c),

-  ``x`` and two hexadecimal digits (like ``7F``),

-  ``u`` and four hexadecimal digits (forming a fixed Unicode escape
   sequence like ``\u005c``),

-  ``u{`` and at least one hexadecimal digit followed by ``}`` (forming
   a bounded Unicode escape sequence like ``\u{5c}``), and

-  any single character except digits from '1' to '9', and characters '``x``',
   '``u``', '``CR``' and '``LF``'.

The examples are provided below:

.. code-block:: typescript
   :linenos:

    let s1 = 'Hello, world!'
    let s2 = "Hello, world!"
    let s3 = "\\"
    let s4 = ""
    let s5 = "don’t worry, be happy"
    let s6 = 'don\'t worry, be happy'
    let s7 = 'don\u0027t worry, be happy'

|

.. _Template Literals:

Template Literals
=================

.. meta:
    frontend_status: Done

Multi-line string literals that can include embedded expressions are called
*template literals*.

A *template* literal with an embedded expression is a *template string*.

A *template string* is not exactly a literal because its value cannot be
evaluated at compile time. The evaluation of a template string is called
*string interpolation* (see :ref:`String Interpolation Expressions`).

.. index::
   string literal
   template literal
   template string
   string interpolation
   multi-line string

.. code-block:: abnf

    TemplateLiteral:
        '`' (BacktickCharacter | embeddedExpression)* '`'
        ;

    BacktickCharacter:
        ~[`\\\r\n\]
        | '\\' EscapeSequence
        | LineContinuation
        ;

See :ref:`String Interpolation Expressions` for the grammar of *embeddedExpression*.

An example of a multi-line string is provided below:

.. code-block:: typescript
   :linenos:

    let sentence = `This is an example of
                    a multi-line string, 
                    which should be enclosed in 
                    backticks`

*Template* literals are of type ``string``, which is a predefined reference
type (see :ref:`Type string`).

|

.. _Null Literal:

``Null`` Literal
================

.. meta:
    frontend_status: Done

*Null literal* is the only literal to denote a reference without pointing
at any entity. It is represented by the keyword ``null``. 

.. code-block:: abnf

    NullLiteral:
        'null' 
        ;

The *null literal* denotes the null reference that represents the absence
of a value. The *null literal* is, by definition, the only value of type
``null`` (see :ref:`Type null`). This value is valid only for types ``T | null``
(see :ref:`Nullish Types`).

.. index::
   null literal
   null reference
   nullish type
   type null

|

.. _Undefined Literal:

``Undefined`` Literal
=====================

.. meta:
    frontend_status: Done

*Undefined literal* is the only literal to denote a reference with a value
that is not defined. *Undefined literal* is the only value of type
``undefined`` (see :ref:`Type undefined`). It is represented by the keyword
``undefined``.

.. code-block:: abnf

    UndefinedLiteral:
        'undefined'
        ;

.. index::
   undefined literal
   type undefined
   keyword

|

.. _Comments:

Comments
********

.. meta:
    frontend_status: Done

*Comment*  is a piece of text added in the stream to document and compliment
the source code. Comments are insignificant for the syntactic grammar.

*Line comments* begin with the sequence of characters '``//``' and end with the
last line separator character. Any character or sequence of characters
between them is allowed but ignored.

*Multi-line comments* begin with the sequence of  characters '``\*``' and end
with the first subsequent sequence of characters '``*/``'. Any character or
sequence of characters between them is allowed but ignored.

Comments cannot be nested.

.. index::
   comment
   syntactic grammar
   multi-line comment

|

.. _Semicolons:

Semicolons
**********

.. meta:
    frontend_status: Done

Declarations and statements are usually terminated by a line separator (see
:ref:`Line Separators`). In some cases, a semicolon must be used to separate
syntax productions written in one line, or to avoid ambiguity.

.. index::
   declaration
   statement
   line separator
   syntax production

.. code-block:: typescript
   :linenos:

    function foo(x: number): number {
        x++;
        x *= x;
        return x
    }

    let i = 1
    i-i++ // one expression
    i;-i++ // two expressions

-------------

.. [1]
   Unicode Standard Version 15.0.0,
   https://www.unicode.org/versions/Unicode15.0.0/

.. [2]
   https://unicode.org/reports/tr31/


.. raw:: pdf

   PageBreak


