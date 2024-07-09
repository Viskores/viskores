==============================
Advanced Types
==============================

:chapref:`base-types:Base Types` introduced some of the base data types defined for use in |VTKm|.
However, for simplicity Chapter :chapref:`base-types:Base Types` just briefly touched the high-level concepts of these types.
In this chapter we dive into much greater depth and introduce several more types.


------------------------------
Single Number Types
------------------------------

As described in Chapter :chapref:`base-types:Base Types`, |VTKm| provides aliases for all the base C types to ensure the representation matches the variable use.
When a specific type width is not required, then the most common types to use are :type:`vtkm::FloatDefault` for floating-point numbers, :type:`vtkm::Id` for array and similar indices, and :type:`vtkm::IdComponent` for shorter-width vector indices.

If a specific type width is desired, then one of the following is used to clearly declare the type and width.

+-------+-----------------------+---------------------+----------------------+
| bytes | floating point        | signed integer      | unsigned integer     |
+=======+=======================+=====================+======================+
|     1 |                       | :type:`vtkm::Int8`  | :type:`vtkm::UInt8`  |
+-------+-----------------------+---------------------+----------------------+
|     2 |                       | :type:`vtkm::Int16` | :type:`vtkm::UInt16` |
+-------+-----------------------+---------------------+----------------------+
|     4 | :type:`vtkm::Float32` | :type:`vtkm::Int32` | :type:`vtkm::UInt32` |
+-------+-----------------------+---------------------+----------------------+
|     8 | :type:`vtkm::Float64` | :type:`vtkm::Int64` | :type:`vtkm::UInt64` |
+-------+-----------------------+---------------------+----------------------+

These |VTKm|--defined types should be preferred over basic C types like ``int`` or ``float``.


------------------------------
Vector Types
------------------------------

Visualization algorithms also often require operations on short vectors.
Arrays indexed in up to three dimensions are common.
Data are often defined in 2-space and 3-space, and transformations are typically done in homogeneous coordinates of length 4.
To simplify these types of operations, |VTKm| provides the :class:`vtkm::Vec` templated type, which is essentially a fixed length array of a given type.

.. doxygenclass:: vtkm::Vec
   :members:

The default constructor of :class:`vtkm::Vec` objects leaves the values uninitialized.
All vectors have a constructor with one argument that is used to initialize all components.
All :class:`vtkm::Vec` objects also have a constructor that allows you to set the individual components (one per argument).
All :class:`vtkm::Vec` objects with a size that is greater than 4 are constructed at run time and support an arbitrary number of initial values.
Likewise, there is a :func:`vtkm::make_Vec` convenience function that builds initialized vector types with an arbitrary number of components.
Once created, you can use the bracket operator to get and set component values with the same syntax as an array.

.. load-example:: CreatingVectorTypes
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Creating vector types.

.. doxygenfunction:: vtkm::make_Vec

The types :type:`vtkm::Id2`, :type:`vtkm::Id3`, and :type:`vtkm::Id4` are type aliases of ``vtkm::Vec<vtkm::Id,2>``, ``vtkm::Vec<vtkm::Id,3>``, and ``vtkm::Vec<vtkm::Id,4>``, respectively.
These are used to index arrays of 2, 3, and 4 dimensions, which is common.
Likewise, :type:`vtkm::IdComponent2`, :type:`vtkm::IdComponent3`, and :type:`vtkm::IdComponent4` are type aliases of ``vtkm::Vec<vtkm::IdComponent,2>``, ``vtkm::Vec<vtkm::IdComponent,3>``, and ``vtkm::Vec<vtkm::IdComponent,4>``, respectively.

Because declaring :class:`vtkm::Vec` with all of its template parameters can be cumbersome, |VTKm| provides easy to use aliases for small vectors of base types.
As introduced in :secref:`base-types:Vector Types`, the following type aliases are available.

+-------+------+------------------------+------------------------+-------------------------+
| bytes | size | floating point         | signed integer         | unsigned integer        |
+=======+======+========================+========================+=========================+
|     1 |    2 |                        | :type:`vtkm::Vec2i_8`  | :type:`vtkm::Vec2ui_8`  |
+-------+------+------------------------+------------------------+-------------------------+
|       |    3 |                        | :type:`vtkm::Vec3i_8`  | :type:`vtkm::Vec3ui_8`  |
+-------+------+------------------------+------------------------+-------------------------+
|       |    4 |                        | :type:`vtkm::Vec4i_8`  | :type:`vtkm::Vec4ui_8`  |
+-------+------+------------------------+------------------------+-------------------------+
|     2 |    2 |                        | :type:`vtkm::Vec2i_16` | :type:`vtkm::Vec2ui_16` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    3 |                        | :type:`vtkm::Vec3i_16` | :type:`vtkm::Vec3ui_16` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    4 |                        | :type:`vtkm::Vec4i_16` | :type:`vtkm::Vec4ui_16` |
+-------+------+------------------------+------------------------+-------------------------+
|     4 |    2 | :type:`vtkm::Vec2f_32` | :type:`vtkm::Vec2i_32` | :type:`vtkm::Vec2ui_32` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    3 | :type:`vtkm::Vec3f_32` | :type:`vtkm::Vec3i_32` | :type:`vtkm::Vec3ui_32` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    4 | :type:`vtkm::Vec4f_32` | :type:`vtkm::Vec4i_32` | :type:`vtkm::Vec4ui_32` |
+-------+------+------------------------+------------------------+-------------------------+
|     8 |    2 | :type:`vtkm::Vec2f_64` | :type:`vtkm::Vec2i_64` | :type:`vtkm::Vec2ui_64` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    3 | :type:`vtkm::Vec3f_64` | :type:`vtkm::Vec3i_64` | :type:`vtkm::Vec3ui_64` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    4 | :type:`vtkm::Vec4f_64` | :type:`vtkm::Vec4i_64` | :type:`vtkm::Vec4ui_64` |
+-------+------+------------------------+------------------------+-------------------------+

:class:`vtkm::Vec` supports component-wise arithmetic using the operators for plus (``+``), minus (``-``), multiply (``*``), and divide (``/``).
It also supports scalar to vector multiplication with the multiply operator.
The comparison operators equal (``==``) is true if every pair of corresponding components are true and not equal (``!=``) is true otherwise.
A special :func:`vtkm::Dot` function is overloaded to provide a dot product for every type of vector.

.. load-example:: VectorOperations
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Vector operations.

These operators, of course, only work if they are also defined for the component type of the :class:`vtkm::Vec`.
For example, the multiply operator will work fine on objects of type ``vtkm::Vec<char,3>``, but the multiply operator will not work on objects of type ``vtkm::Vec<std::string,3>`` because you cannot multiply objects of type ``std::string``.

In addition to generalizing vector operations and making arbitrarily long vectors, :class:`vtkm::Vec` can be repurposed for creating any sequence of homogeneous objects.
Here is a simple example of using :class:`vtkm::Vec` to hold the state of a polygon.

.. load-example:: EquilateralTriangle
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Repurposing a :class:`vtkm::Vec`.

Vec-like Types
==============================

.. index:: Vec-like

The :class:`vtkm::Vec` class provides a convenient structure for holding and passing small vectors of data.
However, there are times when using :class:`vtkm::Vec` is inconvenient or inappropriate.
For example, the size of :class:`vtkm::Vec` must be known at compile time, but there may be need for a vector whose size is unknown until compile time.
Also, the data populating a :class:`vtkm::Vec` might come from a source that makes it inconvenient or less efficient to construct a :class:`vtkm::Vec`.
For this reason, |VTKm| also provides several |Veclike| objects that behave much like :class:`vtkm::Vec` but are a different class.
These |Veclike| objects have the same interface as :class:`vtkm::Vec` except that the ``NUM_COMPONENTS`` constant is not available on those that are sized at run time.
|Veclike| objects also come with a ``CopyInto`` method that will take their contents and copy them into a standard :class:`vtkm::Vec` class.
(The standard :class:`vtkm::Vec` class also has a :func:`vtkm::Vec::CopyInto` method for consistency.)

C-Array Vec Wrapper
------------------------------

The first |Veclike| object is :class:`vtkm::VecC`, which exposes a C-type array as a :class:`vtkm::Vec`.

.. doxygenclass:: vtkm::VecC
   :members:

The constructor for :class:`vtkm::VecC` takes a C array and a size of that array.
There is also a constant version of :class:`vtkm::VecC` named :class:`vtkm::VecCConst`, which takes a constant array and cannot be mutated.

.. doxygenclass:: vtkm::VecCConst
   :members:

The ``vtkm/Types.h`` header defines both :class:`vtkm::VecC` and :class:`vtkm::VecCConst` as well as multiple versions of :func:`vtkm::make_VecC` to easily convert a C array to either a :class:`vtkm::VecC` or :class:`vtkm::VecCConst`.

.. doxygenfunction:: vtkm::make_VecC(T*, vtkm::IdComponent)

.. doxygenfunction:: vtkm::make_VecC(const T *array, vtkm::IdComponent size)

The following example demonstrates converting values from a constant table into a :class:`vtkm::VecCConst` for further consumption.
The table and associated methods define how 8 points come together to form a hexahedron.

.. load-example:: VecCExample
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Using :class:`vtkm::VecCConst` with a constant array.

.. commonerrors::
   The :class:`vtkm::VecC` and :class:`vtkm::VecCConst` classes only hold a pointer to a buffer that contains the data.
   They do not manage the memory holding the data.
   Thus, if the pointer given to :class:`vtkm::VecC` or :class:`vtkm::VecCConst` becomes invalid, then using the object becomes invalid.
   Make sure that the scope of the :class:`vtkm::VecC` or :class:`vtkm::VecCConst` does not outlive the scope of the data it points to.

Variable-Sized Vec
------------------------------

The next |Veclike| object is :class:`vtkm::VecVariable`, which provides a |Veclike| object that can be resized at run time to a maximum value.
Unlike :class:`vtkm::VecC`, :class:`vtkm::VecVariable` holds its own memory, which makes it a bit safer to use.
But also unlike :class:`vtkm::VecC`, you must define the maximum size of :class:`vtkm::VecVariable` at compile time.
Thus, :class:`vtkm::VecVariable` is really only appropriate to use when there is a predetermined limit to the vector size that is fairly small.

.. doxygenclass:: vtkm::VecVariable
   :members:

The following example uses a :class:`vtkm::VecVariable` to store the trace of edges within a hexahedron.
This example uses the methods defined in :numref:`ex:VecVariableExample`.

.. load-example:: VecVariableExample
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Using :class:`vtkm::VecVariable`.

Vecs from Portals
------------------------------

|VTKm| provides further examples of |Veclike| objects as well.
For example, the :class:`vtkm::VecFromPortal` and :class:`vtkm::VecFromPortalPermute` objects allow you to treat a subsection of an arbitrarily large array as a :class:`vtkm::Vec`.
These objects work by attaching to array portals, which are described in
:secref:`basic-array-handles:Array Portals`.

.. doxygenclass:: vtkm::VecFromPortal
   :members:

.. doxygenclass:: vtkm::VecFromPortalPermute
   :members:

Point Coordinate Vec
------------------------------

Another example of a |Veclike| object is :class:`vtkm::VecRectilinearPointCoordinates`, which efficiently represents the point coordinates in an axis-aligned hexahedron.
Such shapes are common in structured grids.
These and other data sets are described in :chapref:`dataset:Data Sets`.

------------------------------
Range
------------------------------

|VTKm| provides a convenience structure named :class:`vtkm::Range` to help manage a range of values.
The :class:`vtkm::Range` ``struct`` contains two data members, :member:`vtkm::Range::Min` and :member:`vtkm::Range::Max`, which represent the ends of the range of numbers.
:member:`vtkm::Range::Min` and :member:`vtkm::Range::Max` are both of type :type:`vtkm::Float64`.
:member:`vtkm::Range::Min` and :member:`vtkm::Range::Max` can be directly accessed, but :class:`vtkm::Range` also comes with several helper functions to make it easier to build and use ranges.
Note that all of these functions treat the minimum and maximum value as inclusive to the range.

.. doxygenstruct:: vtkm::Range
   :members:

The following example demonstrates the operation of :class:`vtkm::Range`.

.. load-example:: UsingRange
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Using :class:`vtkm::Range`.


------------------------------
Bounds
------------------------------

|VTKm| provides a convenience structure named :class:`vtkm::Bounds` to help manage
an axis-aligned region in 3D space. Among other things, this structure is
often useful for representing a bounding box for geometry. The
:class:`vtkm::Bounds` ``struct`` contains three data members,
:member:`vtkm::Bounds::X`, :member:`vtkm::Bounds::Y`, and :member:`vtkm::Bounds::Z`, which represent the range of
the bounds along each respective axis. All three of these members are of
type :class:`vtkm::Range`, which is discussed previously in :secref:`advanced-types:Range`.
:member:`vtkm::Bounds::X`, :member:`vtkm::Bounds::Y`, and :member:`vtkm::Bounds::Z` can
be directly accessed, but :class:`vtkm::Bounds` also comes with the
following helper functions to make it easier to build and use ranges.

.. doxygenstruct:: vtkm::Bounds
   :members:

The following example demonstrates the operation of :class:`vtkm::Bounds`.

.. load-example:: UsingBounds
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Using `vtkm::Bounds`.


------------------------------
Index Ranges
------------------------------

Just as it is sometimes necessary to track a range of real values, there are times when code has to specify a continuous range of values in an index sequence like an array.
For this purpose, |VTKm| provides :class:`RangeId`, which behaves similarly to :class:`Range` except for integer values.

.. doxygenstruct:: vtkm::RangeId
   :members:

|VTKm| also often must operate on 2D and 3D arrays (particularly for structured cell sets).
For these use cases, :class:`RangeId2` and :class:`RangeId3` are provided.

.. doxygenstruct:: vtkm::RangeId2
   :members:

.. doxygenstruct:: vtkm::RangeId3
   :members:


------------------------------
Traits
------------------------------

.. index::
   single: traits
   single: tag

When using templated types, it is often necessary to get information about the type or specialize code based on general properties of the type.
|VTKm| uses *traits* classes to publish and retrieve information about types.
A traits class is simply a templated structure that provides type aliases for tag structures, empty types used for identification.
The traits classes might also contain constant numbers and helpful static functions.
See *Effective C++ Third Edition* by Scott Meyers for a description of traits classes and their uses.

Type Traits
==============================

.. index::
   double: traits; type

The :class:`vtkm::TypeTraits` templated class provides basic information about a core type.
These type traits are available for all the basic C++ types as well as the core |VTKm| types described in :chapref:`base-types:Base Types`.
:class:`vtkm::TypeTraits` contains the following elements.

.. doxygenclass:: vtkm::TypeTraits
   :members:

The :type:`vtkm::TypeTraits::NumericTag` will be an alias for one of the following tags.

.. index::
   triple: tag; type; numeric

.. doxygenstruct:: vtkm::TypeTraitsRealTag

.. doxygenstruct:: vtkm::TypeTraitsIntegerTag

The :type:`vtkm::TypeTraits::DimensionalityTag` will be an alias for one of the following tags.

.. index::
   triple: tag; type; dimensionality

.. doxygenstruct:: vtkm::TypeTraitsScalarTag

.. doxygenstruct:: vtkm::TypeTraitsVectorTag

If for some reason one of these tags do not apply, :type:`vtkm::TypeTraitsUnknownTag` will be used.

.. doxygenstruct:: vtkm::TypeTraitsUnknownTag

The definition of :class:`vtkm::TypeTraits` for :type:`vtkm::Float32` could like something like this.

.. load-example:: TypeTraitsImpl
   :file: GuideExampleTraits.cxx
   :caption: Example definition of ``vtkm::TypeTraits<vtkm::Float32>``.

Here is a simple example of using :class:`vtkm::TypeTraits` to implement a generic function that behaves like the remainder operator (``%``) for all types including floating points and vectors.

.. load-example:: TypeTraits
   :file: GuideExampleTraits.cxx
   :caption: Using :class:`vtkm::TypeTraits` for a generic remainder.

Vector Traits
==============================

.. index::
   double: traits; vector

The templated :class:`vtkm::Vec` class contains several items for introspection (such as the component type and its size).
However, there are other types that behave similarly to :class:`vtkm::Vec` objects but have different ways to perform this introspection.

.. index:: Vec-like

For example, |VTKm| contains |Veclike| objects that essentially behave the same but might have different features.
Also, there may be reason to interchangeably use basic scalar values, like an integer or floating point number, with vectors.
To provide a consistent interface to access these multiple types that represents vectors, the :class:`vtkm::VecTraits` templated class provides information and accessors to vector types.It contains the following elements.

.. doxygenstruct:: vtkm::VecTraits
   :members:

The :type:`vtkm::VecTraits::HasMultipleComponents` could be one of the following tags.

.. index::
   triple: tag; vector; multiple components

.. doxygenstruct:: vtkm::VecTraitsTagMultipleComponents

.. doxygenstruct:: vtkm::VecTraitsTagSingleComponent

The :type:`vtkm::VecTraits::IsSizeStatic` could be one of the following tags.

.. index::
   triple: tag; vector; static

.. doxygenstruct:: vtkm::VecTraitsTagSizeStatic

.. doxygenstruct:: vtkm::VecTraitsTagSizeVariable

The definition of :class:`vtkm::VecTraits` for :type:`vtkm::Id3` could look something like this.

.. load-example:: VecTraitsImpl
   :file: GuideExampleTraits.cxx
   :caption: Example definition of ``vtkm::VecTraits<vtkm::Id3>``.

The real power of vector traits is that they simplify creating generic operations on any type that can look like a vector.
This includes operations on scalar values as if they were vectors of size one.
The following code uses vector traits to simplify the implementation of :index:`less` functors that define an ordering that can be used for sorting and other operations.

.. load-example:: VecTraits
   :file: GuideExampleTraits.cxx
   :caption: Using :class:`vtkm::VecTraits` for less functors.


------------------------------
List Templates
------------------------------

.. index::
   single: lists
   single: template metaprogramming
   single: metaprogramming

|VTKm| internally uses template metaprogramming, which utilizes C++ templates to run source-generating programs, to customize code to various data and compute platforms.
One basic structure often uses with template metaprogramming is a list of class names (also sometimes called a tuple or vector, although both of those names have different meanings in |VTKm|).

Many |VTKm| users only need predefined lists, such as the type lists specified in :secref:`advanced-types:Type Lists`.
Those users can skip most of the details of this section.
However, it is sometimes useful to modify lists, create new lists, or operate on lists, and these usages are documented here.

Building Lists
==============================

A basic list is defined with the :class:`vtkm::List` template.

.. doxygenstruct:: vtkm::List

It is common (but not necessary) to use the ``using`` keyword to define an alias for a list with a particular meaning.

.. load-example:: BaseLists
   :file: GuideExampleLists.cxx
   :caption: Creating lists of types.

|VTKm| defines some special and convenience versions of :class:`vtkm::List`.

.. doxygentypedef:: vtkm::ListEmpty

.. doxygentypedef:: vtkm::ListUniversal

Type Lists
==============================

.. index::
   double: type; lists

One of the major use cases for template metaprogramming lists in |VTKm| is to identify a set of potential data types for arrays.
The :file:`vtkm/TypeList.h` header contains predefined lists for known |VTKm| types.
The following lists are provided.

.. doxygentypedef:: vtkm::TypeListId

.. doxygentypedef:: vtkm::TypeListId2

.. doxygentypedef:: vtkm::TypeListId3

.. doxygentypedef:: vtkm::TypeListId4

.. doxygentypedef:: vtkm::TypeListIdComponent

.. doxygentypedef:: vtkm::TypeListIndex

.. doxygentypedef:: vtkm::TypeListFieldScalar

.. doxygentypedef:: vtkm::TypeListFieldVec2

.. doxygentypedef:: vtkm::TypeListFieldVec3

.. doxygentypedef:: vtkm::TypeListFieldVec4

.. doxygentypedef:: vtkm::TypeListFloatVec

.. doxygentypedef:: vtkm::TypeListField

.. doxygentypedef:: vtkm::TypeListScalarAll

.. doxygentypedef:: vtkm::TypeListBaseC

.. doxygentypedef:: vtkm::TypeListVecCommon

.. doxygentypedef:: vtkm::TypeListVecAll

.. doxygentypedef:: vtkm::TypeListAll

.. doxygentypedef:: vtkm::TypeListCommon

If these lists are not sufficient, it is possible to build new type lists using the existing type lists and the list bases from :secref:`advanced-types:Building Lists` as demonstrated in the following example.

.. load-example:: CustomTypeLists
   :file: GuideExampleLists.cxx
   :caption: Defining new type lists.

The :file:`vtkm/cont/DefaultTypes.h` header defines a macro named :c:macro:`VTKM_DEFAULT_TYPE_LIST` that defines a default list of types to use when, for example, determining the type of a field array.
This macro can change depending on |VTKm| compile options.

Querying Lists
==============================

:file:`vtkm/List.h` contains some templated classes to help get information about a list type.
This are particularly useful for lists that are provided as templated parameters for which you do not know the exact type.

Is a List
------------------------------

The :c:macro:`VTKM_IS_LIST` does a compile-time check to make sure a particular type is actually a :class:`vtkm::List` of types.
If the compile-time check fails, then a build error will occur.
This is a good way to verify that a templated class or method that expects a list actually gets a list.

.. doxygendefine:: VTKM_IS_LIST

.. load-example:: VTKM_IS_LIST
   :file: GuideExampleLists.cxx
   :caption: Checking that a template parameter is a valid :class:`vtkm::List`.

List Size
------------------------------

The size of a list can be determined by using the :type:`vtkm::ListSize` template.
The type of the template will resolve to a ``std::integral_constant<vtkm::IdComponent,N>`` where ``N`` is the number of types in the list.
:type:`vtkm::ListSize` does not work with :type:`vtkm::ListUniversal`.

.. doxygentypedef:: vtkm::ListSize

.. load-example:: ListSize
   :file: GuideExampleLists.cxx
   :caption: Getting the size of a :class:`vtkm::List`.

List Contains
------------------------------

The :type:`vtkm::ListHas` template can be used to determine if a :class:`vtkm::List` contains a particular type.
:type:`vtkm::ListHas` takes two template parameters.
The first parameter is a form of :class:`vtkm::List`.
The second parameter is any type to check to see if it is in the list.
If the type is in the list, then :type:`vtkm::ListHas` resolves to ``std::true_type``.
Otherwise it resolves to ``std::false_type``.
:type:`vtkm::ListHas` always returns true for :type:`vtkm::ListUniversal`.

.. doxygentypedef:: vtkm::ListHas

.. load-example:: ListHas
   :file: GuideExampleLists.cxx
   :caption: Determining if a :class:`vtkm::List` contains a particular type.

List Indices
------------------------------

The :type:`vtkm::ListIndexOf` template can be used to get the index of a particular type in a :class:`vtkm::List`.
:type:`vtkm::ListIndexOf` takes two template parameters.
The first parameter is a form of :class:`vtkm::List`.
The second parameter is any type to check to see if it is in the list.
The type of the template will resolve to a ``std::integral_constant<vtkm::IdComponent,N>`` where ``N`` is the index of the type.
If the requested type is not in the list, then :type:`vtkm::ListIndexOf` becomes ``std::integral_constant<vtkm::IdComponent,-1>``.

.. doxygentypedef:: vtkm::ListIndexOf

Conversely, the :type:`vtkm::ListAt` template can be used to get the type for a particular index.
The two template parameters for :type:`vtkm::ListAt` are the :class:`vtkm::List` and an index for the list.

.. doxygentypedef:: vtkm::ListAt

Neither :type:`vtkm::ListIndexOf` nor :type:`vtkm::ListAt` works with :type:`vtkm::ListUniversal`.

.. load-example:: ListIndices
   :file: GuideExampleLists.cxx
   :caption: Using indices with :class:`vtkm::List`.

Operating on Lists
==============================

In addition to providing the base templates for defining and querying lists, :file:`vtkm/List.h` also contains several features for operating on lists.

Appending Lists
------------------------------

The :type:`vtkm::ListAppend` template joins together 2 or more :class:`vtkm::List` types.
The items are concatenated in the order provided to :type:`vtkm::ListAppend`.
:type:`vtkm::ListAppend` does not work with :type:`vtkm::ListUniversal`.

.. doxygentypedef:: vtkm::ListAppend

.. load-example:: ListAppend
   :file: GuideExampleLists.cxx
   :caption: Appending :class:`vtkm::List` types.

Intersecting Lists
------------------------------

The :type:`vtkm::ListIntersect` template takes two :class:`vtkm::List` types and becomes a :class:`vtkm::List` containing all types in both lists.
If one of the lists is :type:`vtkm::ListUniversal`, the contents of the other list used.

.. doxygentypedef:: vtkm::ListIntersect

.. load-example:: ListIntersect
   :file: GuideExampleLists.cxx
   :caption: Intersecting :class:`vtkm::List` types.

Resolve a Template with all Types in a List
--------------------------------------------------

The :type:`vtkm::ListApply` template transfers all of the types in a :class:`vtkm::List` to another template.
The first template argument of :type:`vtkm::ListApply` is the :class:`vtkm::List` to apply.
The second template argument is another template to apply to.
:type:`vtkm::ListApply` becomes an instance of the passed template with all the types in the :class:`vtkm::List`.
:type:`vtkm::ListApply` can be used to convert a :class:`vtkm::List` to some other template.
:type:`vtkm::ListApply` cannot be used with :type:`vtkm::ListUniversal`.

.. doxygentypedef:: vtkm::ListApply

.. load-example:: ListApply
   :file: GuideExampleLists.cxx
   :caption: Applying a :class:`vtkm::List` to another template.

Transform Each Type in a List
------------------------------

The :type:`vtkm::ListTransform` template applies each item in a :class:`vtkm::List` to another template and constructs a list from all these applications.
The first template argument of :type:`vtkm::ListTransform` is the :class:`vtkm::List` to apply.
The second template argument is another template to apply to.
:type:`vtkm::ListTransform` becomes an instance of a new :class:`vtkm::List` containing the passed template each type.
:type:`vtkm::ListTransform` cannot be used with :type:`vtkm::ListUniversal`.

.. doxygentypedef:: vtkm::ListTransform

.. load-example:: ListTransform
   :file: GuideExampleLists.cxx
   :caption: Transforming a :class:`vtkm::List` using a custom template.

Conditionally Removing Items from a List
----------------------------------------

The :type:`vtkm::ListRemoveIf` template removes items from a :class:`vtkm::List` given a predicate.
The first template argument of :type:`vtkm::ListRemoveIf` is the :class:`vtkm::List`.
The second argument is another template that is used as a predicate to determine if the type should be removed or not.
The predicate should become a type with a ``value`` member that is a static true or false value.
Any type in the list that the predicate evaluates to true is removed.
:type:`vtkm::ListRemoveIf` cannot be used with :type:`vtkm::ListUniversal`.

.. doxygentypedef:: vtkm::ListRemoveIf

.. load-example:: ListRemoveIf
   :file: GuideExampleLists.cxx
   :caption: Removing items from a :class:`vtkm::List`.

Combine all Pairs of Two Lists
------------------------------

The :type:`vtkm::ListCross` takes two lists and performs a cross product of them.
It does this by creating a new :class:`vtkm::List` that contains nested :class:`vtkm::List` types, each of length 2 and containing all possible pairs of items in the first list with items in the second list.
:type:`vtkm::ListCross` is often used in conjunction with another list processing command, such as :type:`vtkm::ListTransform` to build templated types of many combinations.
:type:`vtkm::ListCross` cannot be used with :type:`vtkm::ListUniversal`.

.. doxygentypedef:: vtkm::ListCross

.. load-example:: ListCross
   :file: GuideExampleLists.cxx
   :caption: Creating the cross product of 2 :class:`vtkm::List` types.

Call a Function For Each Type in a List
----------------------------------------

The :type:`vtkm::ListForEach` function  takes a functor object and a :class:`vtkm::List`.
It then calls the functor object with the default object of each type in the list.
This is most typically used with C++ run-time type information to convert a run-time polymorphic object to a statically typed (and possibly inlined) call.

.. doxygenfunction:: vtkm::ListForEach(Functor &&f, vtkm::List<Ts...>, Args&&... args)

The following example shows a rudimentary version of converting a dynamically-typed array to a statically-typed array similar to what is done in |VTKm| classes like :class:`vtkm::cont::UnknownArrayHandle`, which is documented in :chapref:`unknown-array-handle:Unknown Array Handles`.

.. load-example:: ListForEach
   :file: GuideExampleLists.cxx
   :caption: Converting dynamic types to static types with :type:`vtkm::ListForEach`.


------------------------------
Pair
------------------------------

|VTKm| defines a :class:`vtkm::Pair` templated object that behaves just like ``std::pair`` from the standard template library.
The difference is that :class:`vtkm::Pair` will work in both the execution and control environments, whereas the STL ``std::pair`` does not always work in the execution environment.

.. doxygenstruct:: vtkm::Pair
   :members:
   :undoc-members:

The |VTKm| version of :class:`vtkm::Pair` supports the same types, fields, and operations as the STL version.
|VTKm| also provides a :func:`vtkm::make_Pair` function for convenience.

.. doxygenfunction:: vtkm::make_Pair


------------------------------
Tuple
------------------------------

|VTKm| defines a :class:`vtkm::Tuple` templated object that behaves like ``std::tuple`` from the standard template library.
The main difference is that :class:`vtkm::Tuple` will work in both the execution and control environments, whereas the STL ``std::tuple`` does not always work in the execution environment.

.. doxygenclass:: vtkm::Tuple

Defining and Constructing
==============================

:class:`vtkm::Tuple` takes any number of template parameters that define the objects stored the tuple.

.. load-example:: DefineTuple
   :file: GuideExampleTuple.cxx
   :caption: Defining a :class:`vtkm::Tuple`.

You can construct a :class:`vtkm::Tuple` with arguments that will be used to initialize the respective objects.
As a convenience, you can use :func:`vtkm::MakeTuple` to construct a :class:`vtkm::Tuple` of types based on the arguments.

.. doxygenfunction:: vtkm::MakeTuple
.. doxygenfunction:: vtkm::make_tuple

.. load-example:: InitTuple
   :file: GuideExampleTuple.cxx
   :caption: Initializing values in a :class:`vtkm::Tuple`.

Querying
==============================

The size of a :class:`vtkm::Tuple` can be determined by using the :type:`vtkm::TupleSize` template, which resolves to an ``std::integral_constant``.
The types at particular indices can be determined with :type:`vtkm::TupleElement`.

.. doxygentypedef:: vtkm::TupleSize
.. doxygentypedef:: vtkm::TupleElement

.. load-example:: TupleQuery
   :file: GuideExampleTuple.cxx
   :caption: Querying :class:`vtkm::Tuple` types.

The function :func:`vtkm::Get` can be used to retrieve an element from the :class:`vtkm::Tuple`.
:func:`vtkm::Get` returns a reference to the element, so you can set a :class:`vtkm::Tuple` element by setting the return value of :func:`vtkm::Get`.

.. doxygenfunction:: vtkm::Get(const vtkm::Tuple<Ts...> &tuple)
.. doxygenfunction:: vtkm::Get(vtkm::Tuple<Ts...> &tuple)
.. doxygenfunction:: vtkm::get(const vtkm::Tuple<Ts...> &tuple)
.. doxygenfunction:: vtkm::get(vtkm::Tuple<Ts...> &tuple)

.. load-example:: TupleGet
   :file: GuideExampleTuple.cxx
   :caption: Retrieving values from a :class:`vtkm::Tuple`.

For Each Tuple Value
==============================

The :func:`vtkm::ForEach` function takes a tuple and a function or functor and calls the function for each of the items in the tuple.
Nothing is returned from :func:`vtkm::ForEach`, and any return value from the function is ignored.

.. doxygenfunction:: vtkm::ForEach(const vtkm::Tuple<Ts...> &tuple, Function &&f)
.. doxygenfunction:: vtkm::ForEach(vtkm::Tuple<Ts...> &tuple, Function &&f)

:func:`vtkm::ForEach` can be used to check the validity of each item in a :class:`vtkm::Tuple`.

.. load-example:: TupleCheck
   :file: GuideExampleTuple.cxx
   :caption: Using :func:`vtkm::Tuple::ForEach` to check the contents.

:func:`vtkm::ForEach` can also be used to aggregate values in a :class:`vtkm::Tuple`.

.. load-example:: TupleAggregate
   :file: GuideExampleTuple.cxx
   :caption: Using :func:`vtkm::Tuple::ForEach` to aggregate.

The previous examples used an explicit ``struct`` as the functor for clarity.
However, it is often less verbose to use a C++ lambda function.

.. load-example:: TupleAggregateLambda
   :file: GuideExampleTuple.cxx
   :caption: Using :func:`vtkm::Tuple::ForEach` to aggregate.

Transform Each Tuple Value
==============================

The :func:`vtkm::Transform` function builds a new :class:`vtkm::Tuple` by calling a function or functor on each of the items in an existing :class:`vtkm::Tuple`.
The return value is placed in the corresponding part of the resulting :class:`vtkm::Tuple`, and the type is automatically created from the return type of the function.

.. doxygenfunction:: vtkm::Transform(const TupleType &&tuple, Function &&f) -> decltype(Apply(tuple, detail::TupleTransformFunctor(), std::forward<Function>(f)))
.. doxygenfunction:: vtkm::Transform(TupleType &&tuple, Function &&f) -> decltype(Apply(tuple, detail::TupleTransformFunctor(), std::forward<Function>(f)))

.. load-example:: TupleTransform
   :file: GuideExampleTuple.cxx
   :caption: Transforming a :class:`vtkm::Tuple`.

Apply
==============================

The :func:`vtkm::Apply` function calls a function or functor using the objects in a :class:`vtkm::Tuple` as the arguments.
If the function returns a value, that value is returned from :func:`vtkm::Apply`.

.. doxygenfunction:: vtkm::Apply(const vtkm::Tuple<Ts...> &tuple, Function &&f, Args&&... args) -> decltype(tuple.Apply(std::forward<Function>(f), std::forward<Args>(args)...))
.. doxygenfunction:: vtkm::Apply(vtkm::Tuple<Ts...> &tuple, Function &&f, Args&&... args) -> decltype(tuple.Apply(std::forward<Function>(f), std::forward<Args>(args)...))

.. load-example:: TupleApply
   :file: GuideExampleTuple.cxx
   :caption: Applying a :class:`vtkm::Tuple` as arguments to a function.

If additional arguments are given to :func:`vtkm::Apply`, they are also passed to the function (before the objects in the :class:`vtkm::Tuple`).
This is helpful for passing state to the function.

.. load-example:: TupleApplyExtraArgs
   :file: GuideExampleTuple.cxx
   :caption: Using extra arguments with :func:`vtkm::Tuple::Apply`.


.. todo:: Document ``Variant``.


------------------------------
Error Codes
------------------------------

.. index:: error codes

For operations that occur in the control environment, |VTKm| uses exceptions to report errors as described in :chapref:`error-handling:Error Handling`.
However, when operating in the execution environment, it is not feasible to throw exceptions. Thus, for operations designed for the execution environment, the status of an operation that can fail is returned as an :enum:`vtkm::ErrorCode`, which is an ``enum``.

.. doxygenenum:: vtkm::ErrorCode

If a function or method returns an :enum:`vtkm::ErrorCode`, it is a good practice to check to make sure that the returned value is :enumerator:`vtkm::ErrorCode::Success`.
If it is not, you can use the :func:`vtkm::ErrorString` function to convert the :enum:`vtkm::ErrorCode` to a descriptive C string.
The easiest thing to do from within a worklet is to call the worklet's ``RaiseError`` method.

.. doxygenfunction:: vtkm::ErrorString

.. load-example:: HandleErrorCode
   :file: GuideExampleCellLocator.cxx
   :caption: Checking an :enum:`vtkm::ErrorCode` and reporting errors in a worklet.
