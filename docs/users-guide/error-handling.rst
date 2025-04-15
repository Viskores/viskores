==============================
Error Handling
==============================

|Viskores| contains several mechanisms for checking and reporting error conditions.

------------------------------
Runtime Error Exceptions
------------------------------

.. index::
   double: errors; control environment

|Viskores| uses exceptions to report errors.
All exceptions thrown by |Viskores| will be a subclass of :class:`viskores::cont::Error`.
For simple error reporting, it is possible to simply catch a :class:`viskores::cont::Error` and report the error message string reported by the :func:`viskores::cont::Error::GetMessage` method.

.. load-example:: CatchingErrors
   :file: GuideExampleErrorHandling.cxx
   :caption: Simple error reporting.

.. doxygenclass:: viskores::cont::Error
   :members:

There are several subclasses to :class:`viskores::cont::Error`.
The specific subclass gives an indication of the type of error that occurred when the exception was thrown.
Catching one of these subclasses may help a program better recover from errors.

.. doxygenclass:: viskores::cont::ErrorBadAllocation
   :members:

.. doxygenclass:: viskores::cont::ErrorBadDevice
   :members:

.. doxygenclass:: viskores::cont::ErrorBadType
   :members:

.. doxygenclass:: viskores::cont::ErrorBadValue
   :members:

.. doxygenclass:: viskores::cont::ErrorExecution
   :members:

.. doxygenclass:: viskores::cont::ErrorFilterExecution
   :members:

.. doxygenclass:: viskores::cont::ErrorInternal
   :members:

.. doxygenclass:: viskores::cont::ErrorUserAbort
   :members:

.. doxygenclass:: viskores::io::ErrorIO
   :members:


------------------------------
Asserting Conditions
------------------------------

.. index::
   double: errors; assert

In addition to the aforementioned error signaling, the ``viskores/Assert.h`` header file defines a macro named :c:macro:`VISKORES_ASSERT`.
This macro behaves the same as the POSIX :c:macro:`assert` macro.
It takes a single argument that is a condition that is expected to be true.
If it is not true, the program is halted and a message is printed.
Asserts are useful debugging tools to ensure that software is behaving and being used as expected.

.. doxygendefine:: VISKORES_ASSERT

.. load-example:: Assert
   :file: GuideExampleErrorHandling.cxx
   :caption: Using :c:macro:`VISKORES_ASSERT`.

.. didyouknow::
   Like the POSIX :c:macro:`assert`, if the :c:macro:`NDEBUG` macro is defined, then :c:macro:`VISKORES_ASSERT` will become an empty expression.
   Typically :c:macro:`NDEBUG` is defined with a compiler flag (like ``-DNDEBUG``) for release builds to better optimize the code.
   CMake will automatically add this flag for release builds.

.. commonerrors::
   A helpful warning provided by many compilers alerts you of unused variables.
   (This warning is commonly enabled on |Viskores| regression test nightly builds.)
   If a function argument is used only in a :c:macro:`VISKORES_ASSERT`, then it will be required for debug builds and be unused in release builds.
   To get around this problem, add a statement to the function of the form ``(void)variableName;``.
   This statement will have no effect on the code generated but will suppress the warning for release builds.


------------------------------
Compile Time Checks
------------------------------

.. index::
   single: assert; static
   single: static assert

Because |Viskores| makes heavy use of C++ templates, it is possible that these templates could be used with inappropriate types in the arguments.
Using an unexpected type in a template can lead to very confusing errors, so it is better to catch such problems as early as possible.
The :c:macro:`VISKORES_STATIC_ASSERT` macro, defined in ``viskores/StaticAssert.h`` makes this possible.
This macro takes a constant expression that can be evaluated at compile time and verifies that the result is true.

In the following example, :c:macro:`VISKORES_STATIC_ASSERT` and its sister macro :c:macro:`VISKORES_STATIC_ASSERT_MSG`, which allows you to give a descriptive message for the failure, are used to implement checks on a templated function that is designed to work on any scalar type that is represented by 32 or more bits.

.. load-example:: StaticAssert
   :file: GuideExampleErrorHandling.cxx
   :caption: Using :c:macro:`VISKORES_STATIC_ASSERT`.

.. didyouknow::
   In addition to the several trait template classes provided by |Viskores| to introspect C++ types, the C++ standard ``type_traits`` header file contains several helpful templates for general queries on types.
   :numref:`ex:StaticAssert` demonstrates the use of one such template: ``std::is_same``.

.. commonerrors::
   Many templates used to introspect types resolve to the tags ``std::true_type`` and ``std::false_type`` rather than the constant values ``true`` and ``false`` that :c:macro:`VISKORES_STATIC_ASSERT` expects.
   The ``std::true_type`` and ``std::false_type`` tags can be converted to the Boolean literal by adding ``::value`` to the end of them.
   Failing to do so will cause :c:macro:`VISKORES_STATIC_ASSERT` to behave incorrectly.
   :numref:`ex:StaticAssert` demonstrates getting the Boolean literal from the result of ``std::is_same``.
