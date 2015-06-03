### Coding Conventions ###

Several developers contribute to VTK-M and we welcome others who are interested to also contribute to the project. To ensure readability and consistency in the code, we have adopted the following coding conventions. Many of these conventions are adapted from the coding conventions of the VTK project. This is because many of the developers are familiar with VTK coding and because we expect vtkm to have continual interaction with VTK.

+ All code contributed to VTK-M must be compatible with VTK-M’s BSD license.

+ Copyright notices should appear at the top of all source, configuration, and text files.
The statement should have the following form:

```
//==========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//==========================================================================
```

+ The CopyrightStatement checks all files for a similar statement. The test will print out a suggested text that can be copied and pasted to any file that has a missing copyright statement (with appropriate replacement of comment prefix). Exceptions to this copyright statement (for example, third-party files with different but compatible statements) can be added to LICENSE.txt.

+ All include files should use include guards. starting right after the copyright statement. The naming convention of the include guard macro is that it should be all in lower case and start with vtk_m and than continue the path name, starting from the inside the vtkm source code directory, with non alphanumeric characters, such as / and . replaced with underscores. The #endif part of the guard at the bottom of the file should include the guard name in a comment.
 For example, the vtkm/cont/ArrayHandle.h header contains the guard
```cpp
  #ifndef vtk_m_cont_ArrayHandle_h
  #define vtk_m_cont_ArrayHandle_h
```
at the top and
```cpp
  #endif //vtk_m_cont_ArrayHandle_h
```
at the bottom.

The unique use of vtk_m over vtkm is to allow auto-complete engines the ability to differentiate between the header guards and VTKM_ macros that are used within the code base.

+ The VTK-M toolkit has several nested namespaces. The declaration of each namespace should be on its own line, and the code inside the namespace bracket should not be indented. The closing brace at the bottom of the namespace should be documented with a comment identifying the namespace. Namespaces can be grouped as desired. The following is a valid use of namespaces.
```cpp
namespace vtkm {
namespace cont {
namespace detail {
class InternalClass;
} // namespace detail
class ExposedClass;
}
} // namespace vtkm::cont
```

+ Multiple inheritance is not allowed in VTK-M classes.

+ Any functional public class should be in its own header file with the same name as the class. The file should be in a directory that corresponds to the namespace the class is in. There are several exceptions to this rule.
  + Templated classes and template specialization often require the implementation of the class to be broken into pieces. Sometimes a specialization is placed in a header with a different name.
  + Many VTK-M toolkit features are not encapsulated in classes. Functions may be collected by purpose or co-located with associated class.
  + Although tags are technically classes, they do not behave as an enumeration for the compiler. Multiple tags that make up this enumeration are collected together.

+ Some classes, such as vtkm::Tuple are meant to behave as basic types. These are sometimes collected together as if they were related typedefs. The vtkm/Types.h header is a good example of this.

+ The indentation style can be characterized as the “un-indented brace” (allman) style. Indentations are two spaces, and the curly brace (scope delimiter) is placed on the following line and not indented.(i.e. the curly brace lines up with the previous line).
+ Conditional clauses (including loop conditionals such as for and while) must be in braces below the conditional. That is, instead of:
```cpp
  if (test) { clause; }
```
use
```cpp
if (test)
{
  clause;
}
```

The rational for this requirement is to make it obvious whether the clause is executed when stepping through the code with the debugger. The one exception to this rule is when the clause contains a control-flow statement with obvious side effects such as return or break. However, even if the clause contains a single statement and is on the same line, the clause should be surrounded by braces.

+ Use two space indentation.

+ Tabs are not allowed. Only use spaces for indentation. No one can agree on what the size of
a tab stop is, so it is better to not use them at all.

+ There should be no trailing whitespace in any line.

+ Use only alphanumeric characters in names. Use capitalization to demarcate words within a name (camel case). The exception is preprocessor macros and constant numbers that are, by convention, represented in all caps and a single underscore to demarcate words.

+ Namespace names are in all lowercase. They should be a single word that designates its meaning.

+ All class, method, member variable, and functions should start with a capital letter. Local variables should start in lower case and then use camel case. Exceptions can be made when such naming would conflict with previously established conventions in other library. (For example, make Vector2 corresponds to make pair in the standard template library.)

+ Always spell out words in names; do not use abbreviations except in cases where the shortened form is widely understood and a name in its own right (e.g. OpenMP).

+ Always use descriptive names in all identifiers, including local variable names. Particularly avoid meaningless names of a few characters (e.g. x, foo, or tmp) or numbered names with no meaning to the number or order (e.g. value1, value2,...). Also avoid the meaningless for loop variable names i, j, k, etc. Instead, use a name that identifies what type of index is being referenced such as pointIndex, vertexIndex, componentIndex, rowIndex, columnIndex, etc.

+ Classes are documented with Doxygen-style comments before classes, methods, and functions.

+ Exposed classes should not have public instance variables outside of exceptional situations. Access is given by convention through methods with names starting with Set and Get or through overloaded operators.

+ References to classes and functions should be fully qualified with the namespace. This makes it easier to establish classes and functions from different packages and to find source and documentation for the referenced class. As an exception, if one class references an internal or detail class clearly associated with it, the reference can be shortened to internal:: or detail::.

+ use this-> inside of methods when accessing class methods and instance variables to distin- guish between local variables and instance variables.

+ Include statements should generally be in alphabetical order. They can be grouped by pack- age and type.

+ Namespaces should not be brought into global scope or the scope of any VTK-M package names- pace with the “using” keyword. It should also be avoided in class, method, and function scopes (fully qualified namespace references are preferred).

+ All code must be valid by the C++03 and C++11 specifications. It must also compile on older compilers that support C++98. Code that uses language features not available in C++98 must have a second implementation that works around the limitations of C++98. The VTKM_FORCE_ANSI turns on a compiler check for ANSI compatibility in gcc and clang compilers.

+ Limit all lines to 80 characters whenever possible.

+ New code must include regression tests that will run on the dashboards. Generally a new class will have an associated “UnitTest” that will test the operation of the test directly. There may be other tests necessary that exercise the operation with different components or on different architectures.

+ All code must compile and run without error or warning messages on the nightly dashboards, which should include Windows, Mac, and Linux.

+ Use vtkm::Scalar in lieu of float or double to represent data for computation and use vtkm::Id in lieu of int or long for data structure indices. This future-proofs code against changes in precision of the architecture. The indices of vtkm::Tuple are an exception. Using int to reference vtkm::Tuple (and other related classes like vtkm::exec::CellField and vtkm::math::Matrix) indices are acceptable as it is unreasonable to make these vectors longer than the precision of int.

+ All functions and methods defined within the VTK-M toolkit should be declared with VTKM_CONT_EXPORT, VTKM_EXEC_EXPORT, or VTKM_EXEC_CONT_EXPORT.

We should note that although these conventions impose a strict statute on VTK-M coding, these rules (other than those involving licensing and copyright) are not meant to be dogmatic. Examples can be found in the existing code that break these conventions, particularly when the conventions stand in the way of readability (which is the point in having them in the first place). For example, it is often the case that it is more readable for a complicated typedef to stretch a few characters past 80 even if it pushes past the end of a display.
