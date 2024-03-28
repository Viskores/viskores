==============================
Base Types
==============================

It is common for a framework to define its own types.
Even the C++ standard template library defines its own base types like :type:`std::size_t` and :type:`std::pair`.
|VTKm| is no exception.

In fact |VTKm| provides a great many base types.
It is the general coding standard of |VTKm| to not directly use the base C types like ``int`` and ``float`` and instead to use types declared in |VTKm|.
The rational is to precisely declare the representation of each variable to prevent future trouble.

Consider that you are programming something and you need to declare an integer variable.
You would declare this variable as ``int``, right?
Well, maybe.
In C++, the declaration ``int`` does not simply mean "an integer."
``int`` means something much more specific than that.
If you were to look up the C++11 standard, you would find that ``int`` is an integer represented in 32 bits with a two's complement signed representation.
In fact, a C++ compiler has no less than 8 standard integer types.

..
   \footnote{%
     I intentionally use the phrase ``no less than'' for our pedantic readers.
     One could argue that \textcode{char} and \textcode{bool} are treated distinctly by the compiler even if their representations match either \textcode{signed char} or \textcode{unsigned char}.
     Furthermore, many modern C++ compilers have extensions for less universally accepted types like 128-bit integers.
   }

So, ``int`` is nowhere near as general as the code might make it seem, and treating it as such could lead to trouble.
For example, consider the MPI standard, which, back in the 1990's, implicitly selected ``int`` for its indexing needs.
Fast forward to today where there is a need to reference buffers with more than 2 billion elements, but the standard is stuck with a data type that cannot represent sizes that big.
(To be fair, it is *possible* to represent buffers this large in MPI, but it is extraordinarily awkward to do so.

Consequently, we feel that with |VTKm| it is best to declare the intention of a variable with its declaration, which should help both prevent errors and future proof code.
All the types presented in this chapter are declared in :file:`vtkm/Types.h`, which is typically included either directly or indirectly by all source using |VTKm|.

------------------------------
Floating Point Types
------------------------------

|VTKm| declares 2 types to hold floating point numbers: :type:`vtkm::Float32` and :type:`vtkm::Float64`.
These, of course, represent floating point numbers with 32-bits and 64-bits of precision, respectively.
These should be used when the precision of a floating point number is predetermined.

.. doxygentypedef:: vtkm::Float32

.. doxygentypedef:: vtkm::Float64

When the precision of a floating point number is not predetermined, operations usually have to be overloaded or templated to work with multiple precisions.
In cases where a precision must be set, but no particular precision is specified, :type:`vtkm::FloatDefault` should be used.

.. doxygentypedef:: vtkm::FloatDefault

:type:`vtkm::FloatDefault` will be set to either :type:`vtkm::Float32` or :type:`vtkm::Float64` depending on whether the CMake option :cmake:variable:`VTKm_USE_DOUBLE_PRECISION` was set when |VTKm| was compiled, as discussed in :secref:`building:Configuring |VTKm|`.
Using :type:`vtkm::FloatDefault` makes it easier for users to trade off precision and speed.


------------------------------
Integer Types
------------------------------

The most common use of an integer in |VTKm| is to index arrays.
For this purpose, the :type:`vtkm::Id` type should be used.
(The width of :type:`vtkm::Id` is determined by the :cmake:variable:`VTKm_USE_64BIT_IDS` CMake option.)

.. doxygentypedef:: vtkm::Id

|VTKm| also has a secondary index type named :type:`vtkm::IdComponent`, which is smaller and typically used for indexing groups of components within a thread.
For example, if you had an array of 3D points, you would use :type:`vtkm::Id` to reference each point, and you would use :type:`vtkm::IdComponent` to reference the respective :math:`x`, :math:`y`, and :math:`z` components.

.. doxygentypedef:: vtkm::IdComponent

.. index:: std::size_t, size_t
.. didyouknow::
   The |VTKm| index types, :type:`vtkm::Id` and :type:`vtkm::IdComponent` use signed integers.
   This breaks with the convention of other common index types like the C++ standard template library :type:`std::size_t`, which use unsigned integers.
   Unsigned integers make sense for indices as a valid index is always 0 or greater.
   However, doing things like iterating in a for loop backward, representing relative indices, and representing invalid values is much easier with signed integers.
   Thus, |VTKm| chooses to use a signed integer for indexing.

|VTKm| also has types to declare an integer of a specific width and sign.
The types :type:`vtkm::Int8`, :type:`vtkm::Int16`, :type:`vtkm::Int32`, and :type:`vtkm::Int64` specify signed integers of 1, 2, 4, and 8 bytes, respectively.
Likewise, the types :type:`vtkm::UInt8`, :type:`vtkm::UInt16`, :type:`vtkm::UInt32`, and :type:`vtkm::UInt64` specify unsigned integers of 1, 2, 4, and 8 bytes, respectively.

.. doxygentypedef:: vtkm::Int8

.. doxygentypedef:: vtkm::UInt8

.. doxygentypedef:: vtkm::Int16

.. doxygentypedef:: vtkm::UInt16

.. doxygentypedef:: vtkm::Int32

.. doxygentypedef:: vtkm::UInt32

.. doxygentypedef:: vtkm::Int64

.. doxygentypedef:: vtkm::UInt64


------------------------------
Vector Types
------------------------------

Visualization algorithms also often require operations on short vectors.
Arrays indexed in up to three dimensions are common.
Data are often defined in 2-space and 3-space, and transformations are typically done in homogeneous coordinates of length 4.
To simplify these types of operations, |VTKm| provides a collection of base types to represent these short vectors, which are collectively referred to as ``Vec`` types.

:type:`vtkm::Vec2f`, :type:`vtkm::Vec3f`, and :type:`vtkm::Vec4f` specify floating point vectors of 2, 3, and 4 components, respectively.
The precision of the floating point numbers follows that of :type:`vtkm::FloatDefault` (which, as documented in :secref:`base-types:Floating Point Types`, is specified by the :cmake:variable:`VTKm_USE_DOUBLE_PRECISION` compile option).
Components of these and other ``Vec`` types can be references through the ``[ ]`` operator, much like a C array.
A ``Vec`` also supports basic arithmetic operators so that it can be used much like its scalar-value counterparts.

.. doxygentypedef:: vtkm::Vec2f

.. doxygentypedef:: vtkm::Vec3f

.. doxygentypedef:: vtkm::Vec4f

.. load-example:: SimpleVectorTypes
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Simple use of ``Vec`` objects.

You can also specify the precision for each of these vector types by appending the bit size of each component.
For example, :type:`vtkm::Vec3f_32` and :type:`vtkm::Vec3f_64` represent 3-component floating point vectors with each component being 32 bits and 64 bits respectively.
Note that the precision number refers to the precision of each component, not the vector as a whole.
So :type:`vtkm::Vec3f_32` contains 3 32-bit (4-byte) floating point components, which means the entire :type:`vtkm::Vec3f_32` requires 96 bits (12 bytes).

.. doxygentypedef:: vtkm::Vec2f_32

.. doxygentypedef:: vtkm::Vec2f_64

.. doxygentypedef:: vtkm::Vec3f_32

.. doxygentypedef:: vtkm::Vec3f_64

.. doxygentypedef:: vtkm::Vec4f_32

.. doxygentypedef:: vtkm::Vec4f_64

To help with indexing 2-, 3-, and 4- dimensional arrays, |VTKm| provides the types :type:`vtkm::Id2`, :type:`vtkm::Id3`, and :type:`vtkm::Id4`, which are \textidentifier{Vec}s of type :type:`vtkm::Id`.
Likewise, |VTKm| provides :type:`vtkm::IdComponent2`, :type:`vtkm::IdComponent3`, and :type:`vtkm::IdComponent4`.

.. doxygentypedef:: vtkm::Id2

.. doxygentypedef:: vtkm::Id3

.. doxygentypedef:: vtkm::Id4

.. doxygentypedef:: vtkm::IdComponent2

.. doxygentypedef:: vtkm::IdComponent3

.. doxygentypedef:: vtkm::IdComponent4

|VTKm| also provides types for \textidentifier{Vec}s of integers of all varieties described in Section \ref{sec:IntegerTypes}.
:type:`vtkm::Vec2i`, :type:`vtkm::Vec3i`, and :type:`vtkm::Vec4i` are vectors of signed integers whereas :type:`vtkm::Vec2ui`, :type:`vtkm::Vec3ui`, and :type:`vtkm::Vec4ui` are vectors of unsigned integers.
All of these sport components of a width equal to :type:`vtkm::Id`.

.. doxygentypedef:: vtkm::Vec2i

.. doxygentypedef:: vtkm::Vec3i

.. doxygentypedef:: vtkm::Vec4i

.. doxygentypedef:: vtkm::Vec2ui

.. doxygentypedef:: vtkm::Vec3ui

.. doxygentypedef:: vtkm::Vec4ui

The width can be specified by appending the desired number of bits in the same way as the floating point \textidentifier{Vec}s.
For example, :type:`vtkm::Vec4ui_8` is a \textidentifier{Vec} of 4 unsigned bytes.

.. doxygentypedef:: vtkm::Vec2i_8

.. doxygentypedef:: vtkm::Vec2ui_8

.. doxygentypedef:: vtkm::Vec2i_16

.. doxygentypedef:: vtkm::Vec2ui_16

.. doxygentypedef:: vtkm::Vec2i_32

.. doxygentypedef:: vtkm::Vec2ui_32

.. doxygentypedef:: vtkm::Vec2i_64

.. doxygentypedef:: vtkm::Vec2ui_64

.. doxygentypedef:: vtkm::Vec3i_8

.. doxygentypedef:: vtkm::Vec3ui_8

.. doxygentypedef:: vtkm::Vec3i_16

.. doxygentypedef:: vtkm::Vec3ui_16

.. doxygentypedef:: vtkm::Vec3i_32

.. doxygentypedef:: vtkm::Vec3ui_32

.. doxygentypedef:: vtkm::Vec3i_64

.. doxygentypedef:: vtkm::Vec3ui_64

.. doxygentypedef:: vtkm::Vec4i_8

.. doxygentypedef:: vtkm::Vec4ui_8

.. doxygentypedef:: vtkm::Vec4i_16

.. doxygentypedef:: vtkm::Vec4ui_16

.. doxygentypedef:: vtkm::Vec4i_32

.. doxygentypedef:: vtkm::Vec4ui_32

.. doxygentypedef:: vtkm::Vec4i_64

.. doxygentypedef:: vtkm::Vec4ui_64

These types really just scratch the surface of the ``Vec`` types available in |VTKm| and the things that can be done with them.
See :chapref:`advanced-types:Advanced Types` for more information on ``Vec`` types and what can be done with them.
