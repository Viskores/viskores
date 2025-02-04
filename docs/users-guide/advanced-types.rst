==============================
Advanced Types
==============================

:chapref:`base-types:Base Types` introduced some of the base data types defined for use in |Viskores|.
However, for simplicity Chapter :chapref:`base-types:Base Types` just briefly touched the high-level concepts of these types.
In this chapter we dive into much greater depth and introduce several more types.


------------------------------
Single Number Types
------------------------------

As described in Chapter :chapref:`base-types:Base Types`, |Viskores| provides aliases for all the base C types to ensure the representation matches the variable use.
When a specific type width is not required, then the most common types to use are :type:`viskores::FloatDefault` for floating-point numbers, :type:`viskores::Id` for array and similar indices, and :type:`viskores::IdComponent` for shorter-width vector indices.

If a specific type width is desired, then one of the following is used to clearly declare the type and width.

+-------+-----------------------+---------------------+----------------------+
| bytes | floating point        | signed integer      | unsigned integer     |
+=======+=======================+=====================+======================+
|     1 |                       | :type:`viskores::Int8`  | :type:`viskores::UInt8`  |
+-------+-----------------------+---------------------+----------------------+
|     2 |                       | :type:`viskores::Int16` | :type:`viskores::UInt16` |
+-------+-----------------------+---------------------+----------------------+
|     4 | :type:`viskores::Float32` | :type:`viskores::Int32` | :type:`viskores::UInt32` |
+-------+-----------------------+---------------------+----------------------+
|     8 | :type:`viskores::Float64` | :type:`viskores::Int64` | :type:`viskores::UInt64` |
+-------+-----------------------+---------------------+----------------------+

These |Viskores|--defined types should be preferred over basic C types like ``int`` or ``float``.


------------------------------
Vector Types
------------------------------

Visualization algorithms also often require operations on short vectors.
Arrays indexed in up to three dimensions are common.
Data are often defined in 2-space and 3-space, and transformations are typically done in homogeneous coordinates of length 4.
To simplify these types of operations, |Viskores| provides the :class:`viskores::Vec` templated type, which is essentially a fixed length array of a given type.

.. doxygenclass:: viskores::Vec
   :members:

The default constructor of :class:`viskores::Vec` objects leaves the values uninitialized.
All vectors have a constructor with one argument that is used to initialize all components.
All :class:`viskores::Vec` objects also have a constructor that allows you to set the individual components (one per argument).
All :class:`viskores::Vec` objects with a size that is greater than 4 are constructed at run time and support an arbitrary number of initial values.
Likewise, there is a :func:`viskores::make_Vec` convenience function that builds initialized vector types with an arbitrary number of components.
Once created, you can use the bracket operator to get and set component values with the same syntax as an array.

.. load-example:: CreatingVectorTypes
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Creating vector types.

.. doxygenfunction:: viskores::make_Vec

The types :type:`viskores::Id2`, :type:`viskores::Id3`, and :type:`viskores::Id4` are type aliases of ``viskores::Vec<viskores::Id,2>``, ``viskores::Vec<viskores::Id,3>``, and ``viskores::Vec<viskores::Id,4>``, respectively.
These are used to index arrays of 2, 3, and 4 dimensions, which is common.
Likewise, :type:`viskores::IdComponent2`, :type:`viskores::IdComponent3`, and :type:`viskores::IdComponent4` are type aliases of ``viskores::Vec<viskores::IdComponent,2>``, ``viskores::Vec<viskores::IdComponent,3>``, and ``viskores::Vec<viskores::IdComponent,4>``, respectively.

Because declaring :class:`viskores::Vec` with all of its template parameters can be cumbersome, |Viskores| provides easy to use aliases for small vectors of base types.
As introduced in :secref:`base-types:Vector Types`, the following type aliases are available.

+-------+------+------------------------+------------------------+-------------------------+
| bytes | size | floating point         | signed integer         | unsigned integer        |
+=======+======+========================+========================+=========================+
|     1 |    2 |                        | :type:`viskores::Vec2i_8`  | :type:`viskores::Vec2ui_8`  |
+-------+------+------------------------+------------------------+-------------------------+
|       |    3 |                        | :type:`viskores::Vec3i_8`  | :type:`viskores::Vec3ui_8`  |
+-------+------+------------------------+------------------------+-------------------------+
|       |    4 |                        | :type:`viskores::Vec4i_8`  | :type:`viskores::Vec4ui_8`  |
+-------+------+------------------------+------------------------+-------------------------+
|     2 |    2 |                        | :type:`viskores::Vec2i_16` | :type:`viskores::Vec2ui_16` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    3 |                        | :type:`viskores::Vec3i_16` | :type:`viskores::Vec3ui_16` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    4 |                        | :type:`viskores::Vec4i_16` | :type:`viskores::Vec4ui_16` |
+-------+------+------------------------+------------------------+-------------------------+
|     4 |    2 | :type:`viskores::Vec2f_32` | :type:`viskores::Vec2i_32` | :type:`viskores::Vec2ui_32` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    3 | :type:`viskores::Vec3f_32` | :type:`viskores::Vec3i_32` | :type:`viskores::Vec3ui_32` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    4 | :type:`viskores::Vec4f_32` | :type:`viskores::Vec4i_32` | :type:`viskores::Vec4ui_32` |
+-------+------+------------------------+------------------------+-------------------------+
|     8 |    2 | :type:`viskores::Vec2f_64` | :type:`viskores::Vec2i_64` | :type:`viskores::Vec2ui_64` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    3 | :type:`viskores::Vec3f_64` | :type:`viskores::Vec3i_64` | :type:`viskores::Vec3ui_64` |
+-------+------+------------------------+------------------------+-------------------------+
|       |    4 | :type:`viskores::Vec4f_64` | :type:`viskores::Vec4i_64` | :type:`viskores::Vec4ui_64` |
+-------+------+------------------------+------------------------+-------------------------+

:class:`viskores::Vec` supports component-wise arithmetic using the operators for plus (``+``), minus (``-``), multiply (``*``), and divide (``/``).
It also supports scalar to vector multiplication with the multiply operator.
The comparison operators equal (``==``) is true if every pair of corresponding components are true and not equal (``!=``) is true otherwise.
A special :func:`viskores::Dot` function is overloaded to provide a dot product for every type of vector.

.. load-example:: VectorOperations
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Vector operations.

These operators, of course, only work if they are also defined for the component type of the :class:`viskores::Vec`.
For example, the multiply operator will work fine on objects of type ``viskores::Vec<char,3>``, but the multiply operator will not work on objects of type ``viskores::Vec<std::string,3>`` because you cannot multiply objects of type ``std::string``.

In addition to generalizing vector operations and making arbitrarily long vectors, :class:`viskores::Vec` can be repurposed for creating any sequence of homogeneous objects.
Here is a simple example of using :class:`viskores::Vec` to hold the state of a polygon.

.. load-example:: EquilateralTriangle
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Repurposing a :class:`viskores::Vec`.

Vec-like Types
==============================

.. index:: Vec-like

The :class:`viskores::Vec` class provides a convenient structure for holding and passing small vectors of data.
However, there are times when using :class:`viskores::Vec` is inconvenient or inappropriate.
For example, the size of :class:`viskores::Vec` must be known at compile time, but there may be need for a vector whose size is unknown until compile time.
Also, the data populating a :class:`viskores::Vec` might come from a source that makes it inconvenient or less efficient to construct a :class:`viskores::Vec`.
For this reason, |Viskores| also provides several |Veclike| objects that behave much like :class:`viskores::Vec` but are a different class.
These |Veclike| objects have the same interface as :class:`viskores::Vec` except that the ``NUM_COMPONENTS`` constant is not available on those that are sized at run time.
|Veclike| objects also come with a ``CopyInto`` method that will take their contents and copy them into a standard :class:`viskores::Vec` class.
(The standard :class:`viskores::Vec` class also has a :func:`viskores::Vec::CopyInto` method for consistency.)

C-Array Vec Wrapper
------------------------------

The first |Veclike| object is :class:`viskores::VecC`, which exposes a C-type array as a :class:`viskores::Vec`.

.. doxygenclass:: viskores::VecC
   :members:

The constructor for :class:`viskores::VecC` takes a C array and a size of that array.
There is also a constant version of :class:`viskores::VecC` named :class:`viskores::VecCConst`, which takes a constant array and cannot be mutated.

.. doxygenclass:: viskores::VecCConst
   :members:

The ``viskores/Types.h`` header defines both :class:`viskores::VecC` and :class:`viskores::VecCConst` as well as multiple versions of :func:`viskores::make_VecC` to easily convert a C array to either a :class:`viskores::VecC` or :class:`viskores::VecCConst`.

.. doxygenfunction:: viskores::make_VecC(T*, viskores::IdComponent)

.. doxygenfunction:: viskores::make_VecC(const T *array, viskores::IdComponent size)

The following example demonstrates converting values from a constant table into a :class:`viskores::VecCConst` for further consumption.
The table and associated methods define how 8 points come together to form a hexahedron.

.. load-example:: VecCExample
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Using :class:`viskores::VecCConst` with a constant array.

.. commonerrors::
   The :class:`viskores::VecC` and :class:`viskores::VecCConst` classes only hold a pointer to a buffer that contains the data.
   They do not manage the memory holding the data.
   Thus, if the pointer given to :class:`viskores::VecC` or :class:`viskores::VecCConst` becomes invalid, then using the object becomes invalid.
   Make sure that the scope of the :class:`viskores::VecC` or :class:`viskores::VecCConst` does not outlive the scope of the data it points to.

Variable-Sized Vec
------------------------------

The next |Veclike| object is :class:`viskores::VecVariable`, which provides a |Veclike| object that can be resized at run time to a maximum value.
Unlike :class:`viskores::VecC`, :class:`viskores::VecVariable` holds its own memory, which makes it a bit safer to use.
But also unlike :class:`viskores::VecC`, you must define the maximum size of :class:`viskores::VecVariable` at compile time.
Thus, :class:`viskores::VecVariable` is really only appropriate to use when there is a predetermined limit to the vector size that is fairly small.

.. doxygenclass:: viskores::VecVariable
   :members:

The following example uses a :class:`viskores::VecVariable` to store the trace of edges within a hexahedron.
This example uses the methods defined in :numref:`ex:VecVariableExample`.

.. load-example:: VecVariableExample
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Using :class:`viskores::VecVariable`.

Vecs from Portals
------------------------------

|Viskores| provides further examples of |Veclike| objects as well.
For example, the :class:`viskores::VecFromPortal` and :class:`viskores::VecFromPortalPermute` objects allow you to treat a subsection of an arbitrarily large array as a :class:`viskores::Vec`.
These objects work by attaching to array portals, which are described in
:secref:`basic-array-handles:Array Portals`.

.. doxygenclass:: viskores::VecFromPortal
   :members:

.. doxygenclass:: viskores::VecFromPortalPermute
   :members:

Point Coordinate Vec
------------------------------

Another example of a |Veclike| object is :class:`viskores::VecRectilinearPointCoordinates`, which efficiently represents the point coordinates in an axis-aligned hexahedron.
Such shapes are common in structured grids.
These and other data sets are described in :chapref:`dataset:Data Sets`.

------------------------------
Range
------------------------------

|Viskores| provides a convenience structure named :class:`viskores::Range` to help manage a range of values.
The :class:`viskores::Range` ``struct`` contains two data members, :member:`viskores::Range::Min` and :member:`viskores::Range::Max`, which represent the ends of the range of numbers.
:member:`viskores::Range::Min` and :member:`viskores::Range::Max` are both of type :type:`viskores::Float64`.
:member:`viskores::Range::Min` and :member:`viskores::Range::Max` can be directly accessed, but :class:`viskores::Range` also comes with several helper functions to make it easier to build and use ranges.
Note that all of these functions treat the minimum and maximum value as inclusive to the range.

.. doxygenstruct:: viskores::Range
   :members:

The following example demonstrates the operation of :class:`viskores::Range`.

.. load-example:: UsingRange
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Using :class:`viskores::Range`.


------------------------------
Bounds
------------------------------

|Viskores| provides a convenience structure named :class:`viskores::Bounds` to help manage
an axis-aligned region in 3D space. Among other things, this structure is
often useful for representing a bounding box for geometry. The
:class:`viskores::Bounds` ``struct`` contains three data members,
:member:`viskores::Bounds::X`, :member:`viskores::Bounds::Y`, and :member:`viskores::Bounds::Z`, which represent the range of
the bounds along each respective axis. All three of these members are of
type :class:`viskores::Range`, which is discussed previously in :secref:`advanced-types:Range`.
:member:`viskores::Bounds::X`, :member:`viskores::Bounds::Y`, and :member:`viskores::Bounds::Z` can
be directly accessed, but :class:`viskores::Bounds` also comes with the
following helper functions to make it easier to build and use ranges.

.. doxygenstruct:: viskores::Bounds
   :members:

The following example demonstrates the operation of :class:`viskores::Bounds`.

.. load-example:: UsingBounds
   :file: GuideExampleCoreDataTypes.cxx
   :caption: Using `viskores::Bounds`.


------------------------------
Index Ranges
------------------------------

Just as it is sometimes necessary to track a range of real values, there are times when code has to specify a continuous range of values in an index sequence like an array.
For this purpose, |Viskores| provides :class:`RangeId`, which behaves similarly to :class:`Range` except for integer values.

.. doxygenstruct:: viskores::RangeId
   :members:

|Viskores| also often must operate on 2D and 3D arrays (particularly for structured cell sets).
For these use cases, :class:`RangeId2` and :class:`RangeId3` are provided.

.. doxygenstruct:: viskores::RangeId2
   :members:

.. doxygenstruct:: viskores::RangeId3
   :members:


------------------------------
Traits
------------------------------

.. index::
   single: traits
   single: tag

When using templated types, it is often necessary to get information about the type or specialize code based on general properties of the type.
|Viskores| uses *traits* classes to publish and retrieve information about types.
A traits class is simply a templated structure that provides type aliases for tag structures, empty types used for identification.
The traits classes might also contain constant numbers and helpful static functions.
See *Effective C++ Third Edition* by Scott Meyers for a description of traits classes and their uses.

Type Traits
==============================

.. index::
   double: traits; type

The :class:`viskores::TypeTraits` templated class provides basic information about a core type.
These type traits are available for all the basic C++ types as well as the core |Viskores| types described in :chapref:`base-types:Base Types`.
:class:`viskores::TypeTraits` contains the following elements.

.. doxygenclass:: viskores::TypeTraits
   :members:

The :type:`viskores::TypeTraits::NumericTag` will be an alias for one of the following tags.

.. index::
   triple: tag; type; numeric

.. doxygenstruct:: viskores::TypeTraitsRealTag

.. doxygenstruct:: viskores::TypeTraitsIntegerTag

The :type:`viskores::TypeTraits::DimensionalityTag` will be an alias for one of the following tags.

.. index::
   triple: tag; type; dimensionality

.. doxygenstruct:: viskores::TypeTraitsScalarTag

.. doxygenstruct:: viskores::TypeTraitsVectorTag

If for some reason one of these tags do not apply, :type:`viskores::TypeTraitsUnknownTag` will be used.

.. doxygenstruct:: viskores::TypeTraitsUnknownTag

The definition of :class:`viskores::TypeTraits` for :type:`viskores::Float32` could like something like this.

.. load-example:: TypeTraitsImpl
   :file: GuideExampleTraits.cxx
   :caption: Example definition of ``viskores::TypeTraits<viskores::Float32>``.

Here is a simple example of using :class:`viskores::TypeTraits` to implement a generic function that behaves like the remainder operator (``%``) for all types including floating points and vectors.

.. load-example:: TypeTraits
   :file: GuideExampleTraits.cxx
   :caption: Using :class:`viskores::TypeTraits` for a generic remainder.

Vector Traits
==============================

.. index::
   double: traits; vector

The templated :class:`viskores::Vec` class contains several items for introspection (such as the component type and its size).
However, there are other types that behave similarly to :class:`viskores::Vec` objects but have different ways to perform this introspection.

.. index:: Vec-like

For example, |Viskores| contains |Veclike| objects that essentially behave the same but might have different features.
Also, there may be reason to interchangeably use basic scalar values, like an integer or floating point number, with vectors.
To provide a consistent interface to access these multiple types that represents vectors, the :class:`viskores::VecTraits` templated class provides information and accessors to vector types.It contains the following elements.

.. doxygenstruct:: viskores::VecTraits
   :members:

The :type:`viskores::VecTraits::HasMultipleComponents` could be one of the following tags.

.. index::
   triple: tag; vector; multiple components

.. doxygenstruct:: viskores::VecTraitsTagMultipleComponents

.. doxygenstruct:: viskores::VecTraitsTagSingleComponent

The :type:`viskores::VecTraits::IsSizeStatic` could be one of the following tags.

.. index::
   triple: tag; vector; static

.. doxygenstruct:: viskores::VecTraitsTagSizeStatic

.. doxygenstruct:: viskores::VecTraitsTagSizeVariable

The definition of :class:`viskores::VecTraits` for :type:`viskores::Id3` could look something like this.

.. load-example:: VecTraitsImpl
   :file: GuideExampleTraits.cxx
   :caption: Example definition of ``viskores::VecTraits<viskores::Id3>``.

The real power of vector traits is that they simplify creating generic operations on any type that can look like a vector.
This includes operations on scalar values as if they were vectors of size one.
The following code uses vector traits to simplify the implementation of :index:`less` functors that define an ordering that can be used for sorting and other operations.

.. load-example:: VecTraits
   :file: GuideExampleTraits.cxx
   :caption: Using :class:`viskores::VecTraits` for less functors.


------------------------------
List Templates
------------------------------

.. index::
   single: lists
   single: template metaprogramming
   single: metaprogramming

|Viskores| internally uses template metaprogramming, which utilizes C++ templates to run source-generating programs, to customize code to various data and compute platforms.
One basic structure often uses with template metaprogramming is a list of class names (also sometimes called a tuple or vector, although both of those names have different meanings in |Viskores|).

Many |Viskores| users only need predefined lists, such as the type lists specified in :secref:`advanced-types:Type Lists`.
Those users can skip most of the details of this section.
However, it is sometimes useful to modify lists, create new lists, or operate on lists, and these usages are documented here.

Building Lists
==============================

A basic list is defined with the :class:`viskores::List` template.

.. doxygenstruct:: viskores::List

It is common (but not necessary) to use the ``using`` keyword to define an alias for a list with a particular meaning.

.. load-example:: BaseLists
   :file: GuideExampleLists.cxx
   :caption: Creating lists of types.

|Viskores| defines some special and convenience versions of :class:`viskores::List`.

.. doxygentypedef:: viskores::ListEmpty

.. doxygentypedef:: viskores::ListUniversal

Type Lists
==============================

.. index::
   double: type; lists

One of the major use cases for template metaprogramming lists in |Viskores| is to identify a set of potential data types for arrays.
The :file:`viskores/TypeList.h` header contains predefined lists for known |Viskores| types.
The following lists are provided.

.. doxygentypedef:: viskores::TypeListId

.. doxygentypedef:: viskores::TypeListId2

.. doxygentypedef:: viskores::TypeListId3

.. doxygentypedef:: viskores::TypeListId4

.. doxygentypedef:: viskores::TypeListIdComponent

.. doxygentypedef:: viskores::TypeListIndex

.. doxygentypedef:: viskores::TypeListFieldScalar

.. doxygentypedef:: viskores::TypeListFieldVec2

.. doxygentypedef:: viskores::TypeListFieldVec3

.. doxygentypedef:: viskores::TypeListFieldVec4

.. doxygentypedef:: viskores::TypeListFloatVec

.. doxygentypedef:: viskores::TypeListField

.. doxygentypedef:: viskores::TypeListScalarAll

.. doxygentypedef:: viskores::TypeListBaseC

.. doxygentypedef:: viskores::TypeListVecCommon

.. doxygentypedef:: viskores::TypeListVecAll

.. doxygentypedef:: viskores::TypeListAll

.. doxygentypedef:: viskores::TypeListCommon

If these lists are not sufficient, it is possible to build new type lists using the existing type lists and the list bases from :secref:`advanced-types:Building Lists` as demonstrated in the following example.

.. load-example:: CustomTypeLists
   :file: GuideExampleLists.cxx
   :caption: Defining new type lists.

The :file:`viskores/cont/DefaultTypes.h` header defines a macro named :c:macro:`VISKORES_DEFAULT_TYPE_LIST` that defines a default list of types to use when, for example, determining the type of a field array.
This macro can change depending on |Viskores| compile options.

Querying Lists
==============================

:file:`viskores/List.h` contains some templated classes to help get information about a list type.
This are particularly useful for lists that are provided as templated parameters for which you do not know the exact type.

Is a List
------------------------------

The :c:macro:`VISKORES_IS_LIST` does a compile-time check to make sure a particular type is actually a :class:`viskores::List` of types.
If the compile-time check fails, then a build error will occur.
This is a good way to verify that a templated class or method that expects a list actually gets a list.

.. doxygendefine:: VISKORES_IS_LIST

.. load-example:: VISKORES_IS_LIST
   :file: GuideExampleLists.cxx
   :caption: Checking that a template parameter is a valid :class:`viskores::List`.

List Size
------------------------------

The size of a list can be determined by using the :type:`viskores::ListSize` template.
The type of the template will resolve to a ``std::integral_constant<viskores::IdComponent,N>`` where ``N`` is the number of types in the list.
:type:`viskores::ListSize` does not work with :type:`viskores::ListUniversal`.

.. doxygentypedef:: viskores::ListSize

.. load-example:: ListSize
   :file: GuideExampleLists.cxx
   :caption: Getting the size of a :class:`viskores::List`.

List Contains
------------------------------

The :type:`viskores::ListHas` template can be used to determine if a :class:`viskores::List` contains a particular type.
:type:`viskores::ListHas` takes two template parameters.
The first parameter is a form of :class:`viskores::List`.
The second parameter is any type to check to see if it is in the list.
If the type is in the list, then :type:`viskores::ListHas` resolves to ``std::true_type``.
Otherwise it resolves to ``std::false_type``.
:type:`viskores::ListHas` always returns true for :type:`viskores::ListUniversal`.

.. doxygentypedef:: viskores::ListHas

.. load-example:: ListHas
   :file: GuideExampleLists.cxx
   :caption: Determining if a :class:`viskores::List` contains a particular type.

List Indices
------------------------------

The :type:`viskores::ListIndexOf` template can be used to get the index of a particular type in a :class:`viskores::List`.
:type:`viskores::ListIndexOf` takes two template parameters.
The first parameter is a form of :class:`viskores::List`.
The second parameter is any type to check to see if it is in the list.
The type of the template will resolve to a ``std::integral_constant<viskores::IdComponent,N>`` where ``N`` is the index of the type.
If the requested type is not in the list, then :type:`viskores::ListIndexOf` becomes ``std::integral_constant<viskores::IdComponent,-1>``.

.. doxygentypedef:: viskores::ListIndexOf

Conversely, the :type:`viskores::ListAt` template can be used to get the type for a particular index.
The two template parameters for :type:`viskores::ListAt` are the :class:`viskores::List` and an index for the list.

.. doxygentypedef:: viskores::ListAt

Neither :type:`viskores::ListIndexOf` nor :type:`viskores::ListAt` works with :type:`viskores::ListUniversal`.

.. load-example:: ListIndices
   :file: GuideExampleLists.cxx
   :caption: Using indices with :class:`viskores::List`.

Operating on Lists
==============================

In addition to providing the base templates for defining and querying lists, :file:`viskores/List.h` also contains several features for operating on lists.

Appending Lists
------------------------------

The :type:`viskores::ListAppend` template joins together 2 or more :class:`viskores::List` types.
The items are concatenated in the order provided to :type:`viskores::ListAppend`.
:type:`viskores::ListAppend` does not work with :type:`viskores::ListUniversal`.

.. doxygentypedef:: viskores::ListAppend

.. load-example:: ListAppend
   :file: GuideExampleLists.cxx
   :caption: Appending :class:`viskores::List` types.

Intersecting Lists
------------------------------

The :type:`viskores::ListIntersect` template takes two :class:`viskores::List` types and becomes a :class:`viskores::List` containing all types in both lists.
If one of the lists is :type:`viskores::ListUniversal`, the contents of the other list used.

.. doxygentypedef:: viskores::ListIntersect

.. load-example:: ListIntersect
   :file: GuideExampleLists.cxx
   :caption: Intersecting :class:`viskores::List` types.

Resolve a Template with all Types in a List
--------------------------------------------------

The :type:`viskores::ListApply` template transfers all of the types in a :class:`viskores::List` to another template.
The first template argument of :type:`viskores::ListApply` is the :class:`viskores::List` to apply.
The second template argument is another template to apply to.
:type:`viskores::ListApply` becomes an instance of the passed template with all the types in the :class:`viskores::List`.
:type:`viskores::ListApply` can be used to convert a :class:`viskores::List` to some other template.
:type:`viskores::ListApply` cannot be used with :type:`viskores::ListUniversal`.

.. doxygentypedef:: viskores::ListApply

.. load-example:: ListApply
   :file: GuideExampleLists.cxx
   :caption: Applying a :class:`viskores::List` to another template.

Transform Each Type in a List
------------------------------

The :type:`viskores::ListTransform` template applies each item in a :class:`viskores::List` to another template and constructs a list from all these applications.
The first template argument of :type:`viskores::ListTransform` is the :class:`viskores::List` to apply.
The second template argument is another template to apply to.
:type:`viskores::ListTransform` becomes an instance of a new :class:`viskores::List` containing the passed template each type.
:type:`viskores::ListTransform` cannot be used with :type:`viskores::ListUniversal`.

.. doxygentypedef:: viskores::ListTransform

.. load-example:: ListTransform
   :file: GuideExampleLists.cxx
   :caption: Transforming a :class:`viskores::List` using a custom template.

Conditionally Removing Items from a List
----------------------------------------

The :type:`viskores::ListRemoveIf` template removes items from a :class:`viskores::List` given a predicate.
The first template argument of :type:`viskores::ListRemoveIf` is the :class:`viskores::List`.
The second argument is another template that is used as a predicate to determine if the type should be removed or not.
The predicate should become a type with a ``value`` member that is a static true or false value.
Any type in the list that the predicate evaluates to true is removed.
:type:`viskores::ListRemoveIf` cannot be used with :type:`viskores::ListUniversal`.

.. doxygentypedef:: viskores::ListRemoveIf

.. load-example:: ListRemoveIf
   :file: GuideExampleLists.cxx
   :caption: Removing items from a :class:`viskores::List`.

Combine all Pairs of Two Lists
------------------------------

The :type:`viskores::ListCross` takes two lists and performs a cross product of them.
It does this by creating a new :class:`viskores::List` that contains nested :class:`viskores::List` types, each of length 2 and containing all possible pairs of items in the first list with items in the second list.
:type:`viskores::ListCross` is often used in conjunction with another list processing command, such as :type:`viskores::ListTransform` to build templated types of many combinations.
:type:`viskores::ListCross` cannot be used with :type:`viskores::ListUniversal`.

.. doxygentypedef:: viskores::ListCross

.. load-example:: ListCross
   :file: GuideExampleLists.cxx
   :caption: Creating the cross product of 2 :class:`viskores::List` types.

Call a Function For Each Type in a List
----------------------------------------

The :type:`viskores::ListForEach` function  takes a functor object and a :class:`viskores::List`.
It then calls the functor object with the default object of each type in the list.
This is most typically used with C++ run-time type information to convert a run-time polymorphic object to a statically typed (and possibly inlined) call.

.. doxygenfunction:: viskores::ListForEach(Functor &&f, viskores::List<Ts...>, Args&&... args)

The following example shows a rudimentary version of converting a dynamically-typed array to a statically-typed array similar to what is done in |Viskores| classes like :class:`viskores::cont::UnknownArrayHandle`, which is documented in :chapref:`unknown-array-handle:Unknown Array Handles`.

.. load-example:: ListForEach
   :file: GuideExampleLists.cxx
   :caption: Converting dynamic types to static types with :type:`viskores::ListForEach`.


------------------------------
Pair
------------------------------

|Viskores| defines a :class:`viskores::Pair` templated object that behaves just like ``std::pair`` from the standard template library.
The difference is that :class:`viskores::Pair` will work in both the execution and control environments, whereas the STL ``std::pair`` does not always work in the execution environment.

.. doxygenstruct:: viskores::Pair
   :members:
   :undoc-members:

The |Viskores| version of :class:`viskores::Pair` supports the same types, fields, and operations as the STL version.
|Viskores| also provides a :func:`viskores::make_Pair` function for convenience.

.. doxygenfunction:: viskores::make_Pair


------------------------------
Tuple
------------------------------

|Viskores| defines a :class:`viskores::Tuple` templated object that behaves like ``std::tuple`` from the standard template library.
The main difference is that :class:`viskores::Tuple` will work in both the execution and control environments, whereas the STL ``std::tuple`` does not always work in the execution environment.

.. doxygenclass:: viskores::Tuple

Defining and Constructing
==============================

:class:`viskores::Tuple` takes any number of template parameters that define the objects stored the tuple.

.. load-example:: DefineTuple
   :file: GuideExampleTuple.cxx
   :caption: Defining a :class:`viskores::Tuple`.

You can construct a :class:`viskores::Tuple` with arguments that will be used to initialize the respective objects.
As a convenience, you can use :func:`viskores::MakeTuple` to construct a :class:`viskores::Tuple` of types based on the arguments.

.. doxygenfunction:: viskores::MakeTuple
.. doxygenfunction:: viskores::make_tuple

.. load-example:: InitTuple
   :file: GuideExampleTuple.cxx
   :caption: Initializing values in a :class:`viskores::Tuple`.

Querying
==============================

The size of a :class:`viskores::Tuple` can be determined by using the :type:`viskores::TupleSize` template, which resolves to an ``std::integral_constant``.
The types at particular indices can be determined with :type:`viskores::TupleElement`.

.. doxygentypedef:: viskores::TupleSize
.. doxygentypedef:: viskores::TupleElement

.. load-example:: TupleQuery
   :file: GuideExampleTuple.cxx
   :caption: Querying :class:`viskores::Tuple` types.

The function :func:`viskores::Get` can be used to retrieve an element from the :class:`viskores::Tuple`.
:func:`viskores::Get` returns a reference to the element, so you can set a :class:`viskores::Tuple` element by setting the return value of :func:`viskores::Get`.

.. doxygenfunction:: viskores::Get(const viskores::Tuple<Ts...> &tuple)
.. doxygenfunction:: viskores::Get(viskores::Tuple<Ts...> &tuple)
.. doxygenfunction:: viskores::get(const viskores::Tuple<Ts...> &tuple)
.. doxygenfunction:: viskores::get(viskores::Tuple<Ts...> &tuple)

.. load-example:: TupleGet
   :file: GuideExampleTuple.cxx
   :caption: Retrieving values from a :class:`viskores::Tuple`.

For Each Tuple Value
==============================

The :func:`viskores::ForEach` function takes a tuple and a function or functor and calls the function for each of the items in the tuple.
Nothing is returned from :func:`viskores::ForEach`, and any return value from the function is ignored.

.. doxygenfunction:: viskores::ForEach(const viskores::Tuple<Ts...> &tuple, Function &&f)
.. doxygenfunction:: viskores::ForEach(viskores::Tuple<Ts...> &tuple, Function &&f)

:func:`viskores::ForEach` can be used to check the validity of each item in a :class:`viskores::Tuple`.

.. load-example:: TupleCheck
   :file: GuideExampleTuple.cxx
   :caption: Using :func:`viskores::Tuple::ForEach` to check the contents.

:func:`viskores::ForEach` can also be used to aggregate values in a :class:`viskores::Tuple`.

.. load-example:: TupleAggregate
   :file: GuideExampleTuple.cxx
   :caption: Using :func:`viskores::Tuple::ForEach` to aggregate.

The previous examples used an explicit ``struct`` as the functor for clarity.
However, it is often less verbose to use a C++ lambda function.

.. load-example:: TupleAggregateLambda
   :file: GuideExampleTuple.cxx
   :caption: Using :func:`viskores::Tuple::ForEach` to aggregate.

Transform Each Tuple Value
==============================

The :func:`viskores::Transform` function builds a new :class:`viskores::Tuple` by calling a function or functor on each of the items in an existing :class:`viskores::Tuple`.
The return value is placed in the corresponding part of the resulting :class:`viskores::Tuple`, and the type is automatically created from the return type of the function.

.. doxygenfunction:: viskores::Transform(const TupleType &&tuple, Function &&f) -> decltype(Apply(tuple, detail::TupleTransformFunctor(), std::forward<Function>(f)))
.. doxygenfunction:: viskores::Transform(TupleType &&tuple, Function &&f) -> decltype(Apply(tuple, detail::TupleTransformFunctor(), std::forward<Function>(f)))

.. load-example:: TupleTransform
   :file: GuideExampleTuple.cxx
   :caption: Transforming a :class:`viskores::Tuple`.

Apply
==============================

The :func:`viskores::Apply` function calls a function or functor using the objects in a :class:`viskores::Tuple` as the arguments.
If the function returns a value, that value is returned from :func:`viskores::Apply`.

.. doxygenfunction:: viskores::Apply(const viskores::Tuple<Ts...> &tuple, Function &&f, Args&&... args) -> decltype(tuple.Apply(std::forward<Function>(f), std::forward<Args>(args)...))
.. doxygenfunction:: viskores::Apply(viskores::Tuple<Ts...> &tuple, Function &&f, Args&&... args) -> decltype(tuple.Apply(std::forward<Function>(f), std::forward<Args>(args)...))

.. load-example:: TupleApply
   :file: GuideExampleTuple.cxx
   :caption: Applying a :class:`viskores::Tuple` as arguments to a function.

If additional arguments are given to :func:`viskores::Apply`, they are also passed to the function (before the objects in the :class:`viskores::Tuple`).
This is helpful for passing state to the function.

.. load-example:: TupleApplyExtraArgs
   :file: GuideExampleTuple.cxx
   :caption: Using extra arguments with :func:`viskores::Tuple::Apply`.


.. todo:: Document ``Variant``.


------------------------------
Error Codes
------------------------------

.. index:: error codes

For operations that occur in the control environment, |Viskores| uses exceptions to report errors as described in :chapref:`error-handling:Error Handling`.
However, when operating in the execution environment, it is not feasible to throw exceptions. Thus, for operations designed for the execution environment, the status of an operation that can fail is returned as an :enum:`viskores::ErrorCode`, which is an ``enum``.

.. doxygenenum:: viskores::ErrorCode

If a function or method returns an :enum:`viskores::ErrorCode`, it is a good practice to check to make sure that the returned value is :enumerator:`viskores::ErrorCode::Success`.
If it is not, you can use the :func:`viskores::ErrorString` function to convert the :enum:`viskores::ErrorCode` to a descriptive C string.
The easiest thing to do from within a worklet is to call the worklet's ``RaiseError`` method.

.. doxygenfunction:: viskores::ErrorString

.. load-example:: HandleErrorCode
   :file: GuideExampleCellLocator.cxx
   :caption: Checking an :enum:`viskores::ErrorCode` and reporting errors in a worklet.
