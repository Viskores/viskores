### Coding Conventions ###

Several developers contribute to Viskores and we welcome others who are
interested to also contribute to the project. To ensure readability and
consistency in the code, we have adopted the following coding conventions.
Many of these conventions are adapted from the coding conventions of the
VTK project. This is because many of the developers are familiar with VTK
coding and because we expect viskores to have continual interaction with VTK.

  + All code contributed to Viskores must be compatible with Viskores’s BSD
    license.

  + License notices should appear at the top of all source,
    configuration, and text files. The statement should have the following
    form. Various copyright specific details are referenced in the LICENSE.txt
    file.

```
//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

```

  + The LicenseStatement test checks all files for a similar statement.
    The test will print out a suggested text that can be copied and pasted
    to any file that has a missing copyright statement (with appropriate
    replacement of comment prefix). Exceptions to this copyright statement
    (for example, third-party files with different but compatible
    statements) can be added to DCO.txt.

  + All include files should use include guards. starting right after the
    copyright statement. The naming convention of the include guard macro
    is that it should be all in lower case and start with viskores and than
    continue the path name, starting from the inside the viskores source code
    directory, with non alphanumeric characters, such as / and . replaced
    with underscores. The `#endif` part of the guard at the bottom of the
    file should include the guard name in a comment. For example, the
    viskores/cont/ArrayHandle.h header contains the guard demostrated below.

    The unique use of viskores over viskores is to allow auto-complete engines the
    ability to differentiate between the header guards and VISKORES_ macros
    that are used within the code base.

```cpp
#ifndef viskores_cont_ArrayHandle_h
#define viskores_cont_ArrayHandle_h

// All ArrayHandle code here

#endif //viskores_cont_ArrayHandle_h
```

  + The Viskores toolkit has several nested namespaces. The declaration of
    each namespace should be on its own line, and the code inside the
    namespace bracket should not be indented. The closing brace at the
    bottom of the namespace should be documented with a comment identifying
    the namespace. Namespaces can be grouped as desired. The following is a
    valid use of namespaces.

```cpp
namespace viskores {
namespace cont {
namespace detail {
class InternalClass;
} // namespace detail
class ExposedClass;
}
} // namespace viskores::cont
```

  + Multiple inheritance is not allowed in Viskores classes.

  + Any functional public class should be in its own header file with the
    same name as the class. The file should be in a directory that
    corresponds to the namespace the class is in. There are several
    exceptions to this rule.
      + Templated classes and template specialization often require the
        implementation of the class to be broken into pieces. Sometimes a
        specialization is placed in a header with a different name.
      + Many Viskores toolkit features are not encapsulated in classes.
        Functions may be collected by purpose or co-located with associated
        class.
      + Although tags are technically classes, they behave as an
        enumeration for the compiler. Multiple tags that make up this
        enumeration are collected together.
      + Some classes, such as `viskores::Vec` are meant to behave as basic
        types. These are sometimes collected together as if they were
        related typedefs. The viskores/Types.h header is a good example of
        this.

  + The indentation style can be characterized as [Allman Style].
    With the curly brace (scope delimiter) placed on the
    following line and not-indented.

```cpp
if (test)
{
  clause;
}
```

  + Conditional clauses including loop conditionals such as for and while
    must be in braces below the conditional.
    
```cpp
for (auto v : vector)
{
  single line clause;
}
```

  + Two space indentation. Tabs are not allowed. Trailing whitespace
    is not allowed.

  + Code formatting is strictly enforced. Viskores's [development workflow]
    includes a reformatting step that compares the formatting of new source
    code with our formatting definitions. Incompatible code can be
    automatically reformatted to be compliant. See [CONTRIBUTING.md -->
    Reformat a Topic][Reformat] for details.

[development workflow]: ../CONTRIBUTING.md#workflow
[reformat]:             ../CONTRIBUTING.md#reformat-a-topic

  + Use only alphanumeric characters in names. Use capitalization to
    demarcate words within a name (camel case). The exception is
    preprocessor macros and constant numbers that are, by convention,
    represented in all caps and a single underscore to demarcate words.

  + Namespace names are in all lowercase. They should be a single word that
    designates its meaning.

  + All class, method, member variable, and functions should start with a
    capital letter. Local variables should start in lower case and then use
    camel case. Exceptions can be made when such naming would conflict with
    previously established conventions in other library. (For example,
    `make_Vector2` corresponds to make pair in the standard template
    library.)

  + Always spell out words in names; do not use abbreviations except in
    cases where the shortened form is widely understood and a name in its
    own right (e.g. OpenMP).

  + Always use descriptive names in all identifiers, including local
    variable names. Particularly avoid meaningless names of a few
    characters (e.g. `x`, `foo`, or `tmp`) or numbered names with no
    meaning to the number or order (e.g. `value1`, `value2`,...). Also
    avoid the meaningless for loop variable names `i`, `j`, `k`, etc.
    Instead, use a name that identifies what type of index is being
    referenced such as `pointIndex`, `vertexIndex`, `componentIndex`,
    `rowIndex`, `columnIndex`, etc.

  + Classes are documented with Doxygen-style comments before classes,
    methods, and functions.

  + Exposed classes should not have public instance variables outside of
    exceptional situations. Access is given by convention through methods
    with names starting with Set and Get or through overloaded operators.

  + References to classes and functions should be fully qualified with the
    namespace. This makes it easier to establish classes and functions from
    different packages and to find source and documentation for the
    referenced class. As an exception, if one class references an internal
    or detail class clearly associated with it, the reference can be
    shortened to `internal::` or `detail::`.

  + Use `this->` inside of methods when accessing class methods and
    instance variables to distinguish between local variables and instance
    variables.

  + Include statements should generally be in alphabetical order. They can
    be grouped by package and type.

  + Namespaces should not be brought into global scope or the scope of any
    Viskores package namespace with the `using` keyword. It should also be
    avoided in class, method, and function scopes (fully qualified
    namespace references are preferred).

  + All code must be valid by the 7 specifications.

  + New code must include regression tests that will run on the dashboards.
    Generally a new class will have an associated "UnitTest" that will test
    the operation of the test directly. There may be other tests necessary
    that exercise the operation with different components or on different
    architectures.

  + All code must compile and run without error or warning messages on the
    nightly dashboards, which include Windows, Mac, and Linux.

  + Do not use the base C types like `float`, `double`, `int`, `long`, etc.
    When appropriate, use templates to determine the correct type.
    Otherwise, use types defined by Viskores. Use `viskores::Id` or
    `viskores::IdComponent` for indices and sizes. Consider using
    `viskores::FloatDefault` makes sense. Otherwise, use one of Viskores's types
    do be explicit about the data type. These are `viskores::Int8`,
    `viskores::UInt8`, `viskores::Int16`, `viskores::UInt16`, `viskores::Int32`,
    `viskores::UInt32`, `viskores::Float32`, `viskores::Int64`, `viskores::UInt64`, and
    `viskores::Float64`.

  + All functions and methods defined within the Viskores toolkit should be
    declared with `VISKORES_CONT`, `VISKORES_EXEC`, or `VISKORES_EXEC_CONT`.

We should note that although these conventions impose a strict statute on
Viskores coding, these rules (other than those involving licensing and
copyright) are not meant to be dogmatic. Examples can be found in the
existing code that break these conventions, particularly when the
conventions stand in the way of readability (which is the point in having
them in the first place).

[Allman Style]:  https://en.wikipedia.org/wiki/Indent_style#Allman_style
