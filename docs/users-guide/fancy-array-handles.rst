==============================
Fancy Array Handles
==============================

.. todo:: Document :class:`vtkm::cont::ArrayHandleMultiplexer`.

.. index::
   double: array handle; fancy

One of the features of using :class:`vtkm::cont::ArrayHandle` is that it hides the implementation and layout of the array behind a generic interface.
This gives us the opportunity to replace a simple C array with some custom definition of the data and the code using the :class:`vtkm::cont::ArrayHandle` is none the wiser.

This gives us the opportunity to implement *fancy* arrays that do more than simply look up a value in an array.
For example, arrays can be augmented on the fly by mutating their indices or values.
Or values could be computed directly from the index so that no storage is required for the array at all.
|VTKm| provides many of the fancy arrays, which we explore in this section.

.. didyouknow::
   One of the advantages of |VTKm|'s implementation of fancy arrays is that they can define whole arrays without actually storing and values.
   For example, :class:`vtkm::cont::ArrayHandleConstant`, :class:`vtkm::cont::ArrayHandleIndex`, and :class:`vtkm::cont::ArrayHandleCounting` do not store data in any array in memory.
   Rather, they construct the value for an index at runtime.
   Likewise, arrays like :class:`vtkm::cont::ArrayHandlePermutation` construct new arrays from the values of other arrays without having to create a copy of the data.

.. didyouknow::
   This chapter documents several array handle types that modify other array handles.
   :chapref:`memory-layout:Memory Layout of Array Handles` has several similar examples of modifying basic arrays to represent data in different layouts.
   The difference is that the fancy array handles in this chapter decorate other array handles of any type whereas those in :numref:`Chapter {number} <memory-layout:Memory Layout of Array Handles>` only decorate basic array handles.
   If you do not find the fancy array handle you are looking for here, you might try that chapter.


------------------------------
Constant Arrays
------------------------------

.. index::
   single: array handle; constant
   single: constant array handle

A constant array is a fancy array handle that has the same value in all of its entries.
The constant array provides this array without actually using any memory.

Specifying a constant array in |VTKm| is straightforward.
|VTKm| has a class named :class:`vtkm::cont::ArrayHandleConstant`.
:class:`vtkm::cont::ArrayHandleConstant` is a templated class with a single template argument that is the type of value for each element in the array.
The constructor for :class:`vtkm::cont::ArrayHandleConstant` takes the value to provide by the array and the number of values the array should present.
The following example is a simple demonstration of the constant array handle.

.. doxygenclass:: vtkm::cont::ArrayHandleConstant
   :members:

.. load-example:: ArrayHandleConstant
   :file: GuideExampleArrayHandleConstant.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleConstant`.

The :file:`vtkm/cont/ArrayHandleConstant.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandleConstant` that takes a value and a size for the array.
This function can sometimes be used to avoid having to declare the full array type.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleConstant

.. load-example:: MakeArrayHandleConstant
   :file: GuideExampleArrayHandleConstant.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleConstant`.


------------------------------
Viewing a Subset of an Array
------------------------------

.. index::
   single: array handle; view
   single: view array handle

An array handle view is a fancy array handle that returns a subset of an already existing array handle.
The array handle view uses the same memory as the existing array handle the view was created from.
This means that changes to the data in the array handle view will also change the data in the original array handle.

.. doxygenclass:: vtkm::cont::ArrayHandleView
   :members:

To use the :class:`vtkm::cont::ArrayHandleView` you must supply an :class:`vtkm::cont::ArrayHandle` to the :class:`vtkm::cont::ArrayHandleView` class constructor.
:class:`vtkm::cont::ArrayHandleView` is a templated class with a single template argument that is the :class:`vtkm::cont::ArrayHandle` type of the array that the view is being created from.
The constructor for :class:`vtkm::cont::ArrayHandleView` takes a target array, starting index, and length.
The following example shows a simple usage of the array handle view.

.. load-example:: ArrayHandleView
   :file: GuideExampleArrayHandleView.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleView`.

The :file:`vtkm/cont/ArrayHandleView.h` header contains a templated convenience function :func:`vtkm::cont::make_ArrayHandleView` that takes a target array, index, and length.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleView

.. load-example:: MakeArrayHandleView
   :file: GuideExampleArrayHandleView.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleView`.


------------------------------
Counting Arrays
------------------------------

.. index::
   single: array handle; counting
   single: counting array handle
   single: array handle; index
   single: index array handle

A counting array is a fancy array handle that provides a sequence of numbers.
These fancy arrays can represent the data without actually using any memory.

|VTKm| provides two versions of a counting array.
The first version is an index array that provides a specialized but common form of a counting array called an index array.
An index array has values of type :type:`vtkm::Id` that start at 0 and count up by 1 (i.e., :math:`0, 1, 2, 3,\ldots`).
The index array mirrors the array's index.

.. doxygenclass:: vtkm::cont::ArrayHandleIndex
   :members:

Specifying an index array in |VTKm| is done with a class named :class:`vtkm::cont::ArrayHandleIndex`.
The constructor for :class:`vtkm::cont::ArrayHandleIndex` takes the size of the array to create.
The following example is a simple demonstration of the index array handle.

.. load-example:: ArrayHandleIndex
   :file: GuideExampleArrayHandleCounting.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleIndex`.

A :func:`vtkm::cont::make_ArrayHandleIndex` convenience function is also available.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleIndex

The :class:`vtkm::cont::ArrayHandleCounting` class provides a more general form of counting.
:class:`vtkm::cont::ArrayHandleCounting` is a templated class with a single template argument that is the type of value for each element in the array.
The constructor for :class:`vtkm::cont::ArrayHandleCounting` takes three arguments: the start value (used at index 0), the step from one value to the next, and the length of the array.
The following example is a simple demonstration of the counting array handle.

.. doxygenclass:: vtkm::cont::ArrayHandleCounting
   :members:

.. load-example:: ArrayHandleCountingBasic
   :file: GuideExampleArrayHandleCounting.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleCounting`.

.. didyouknow::
   In addition to being simpler to declare, :class:`vtkm::cont::ArrayHandleIndex` is slightly faster than :class:`vtkm::cont::ArrayHandleCounting`.
   Thus, when applicable, you should prefer using :class:`vtkm::cont::ArrayHandleIndex`.

The :file:`vtkm/cont/ArrayHandleCounting.h` header also contains the templated convenience function :file:`vtkm::cont::make_ArrayHandleCounting` that also takes the start value, step, and length as arguments.
This function can sometimes be used to avoid having to declare the full array type.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleCounting

.. load-example:: MakeArrayHandleCountingBasic
   :file: GuideExampleArrayHandleCounting.cxx
   :caption: Using :file:`vtkm::cont::make_ArrayHandleCounting`.

There are no fundamental limits on how :class:`vtkm::cont::ArrayHandleCounting` counts.
For example, it is possible to count backwards.

.. load-example:: ArrayHandleCountingBackward
   :file: GuideExampleArrayHandleCounting.cxx
   :caption: Counting backwards with :class:`vtkm::cont::ArrayHandleCounting`.

It is also possible to use :class:`vtkm::cont::ArrayHandleCounting` to make sequences of :class:`vtkm::Vec` values with piece-wise counting in each of the components.

.. load-example:: ArrayHandleCountingVec
   :file: GuideExampleArrayHandleCounting.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleCounting` with :class:`vtkm::Vec` objects.


------------------------------
Cast Arrays
------------------------------

.. index::
   single: array handle; cast
   single: cast array handle

A cast array is a fancy array that changes the type of the elements in an array.
The cast array provides this re-typed array without actually copying or generating any data.
Instead, casts are performed as the array is accessed.

|VTKm| has a class named :class:`vtkm::cont::ArrayHandleCast` to perform this implicit casting.
:class:`vtkm::cont::ArrayHandleCast` is a templated class with two template arguments.
The first argument is the type to cast values to.
The second argument is the type of the original :class:`vtkm::cont::ArrayHandle`.
The constructor to :class:`vtkm::cont::ArrayHandleCast` takes the :class:`vtkm::cont::ArrayHandle` to modify by casting.

.. doxygenclass:: vtkm::cont::ArrayHandleCast
   :members:

.. load-example:: ArrayHandleCast
   :file: GuideExampleArrayHandleCast.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleCast`.

The :file:`vtkm/cont/ArrayHandleCast.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandleCast` that constructs the cast array.
The first argument is the original :class:`vtkm::cont::ArrayHandle` original array to cast.
The optional second argument is of the type to cast to (or you can optionally specify the cast-to type as a template argument.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleCast

.. load-example:: MakeArrayHandleCast
   :file: GuideExampleArrayHandleCast.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleCast`.


------------------------------
Discard Arrays
------------------------------

.. index::
   single: array handle; discard
   single discard array handle

It is sometimes the case where you will want to run an operation in |VTKm| that fills values in two (or more) arrays, but you only want the values that are stored in one of the arrays.
It is possible to allocate space for both arrays and then throw away the values that you do not want, but that is a waste of memory.
It is also possible to rewrite the functionality to output only what you want, but that is a poor use of developer time.

To solve this problem easily, |VTKm| provides :class:`vtkm::cont::ArrayHandleDiscard`.
This array behaves similar to a regular :class:`vtkm::cont::ArrayHandle` in that it can be "allocated" and has size, but any values that are written to it are immediately discarded.
:class:`vtkm::cont::ArrayHandleDiscard` takes up no memory.

.. doxygenclass:: vtkm::cont::ArrayHandleDiscard
   :members:

.. load-example:: ArrayHandleDiscard
   :file: GuideExampleArrayHandleDiscard.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleDiscard`.}


------------------------------
Permuted Arrays
------------------------------

.. index::
   single: array handle; permutation
   single: permutation array handle

A permutation array is a fancy array handle that reorders the elements in an array.
Elements in the array can be skipped over or replicated.
The permutation array provides this reordered array without actually coping any data.
Instead, indices are adjusted as the array is accessed.

Specifying a permutation array in |VTKm| is straightforward.
|VTKm| has a class named :class:`vtkm::cont::ArrayHandlePermutation` that takes two arrays: an array of values and an array of indices that maps an index in the permutation to an index of the original values.
The index array is specified first.
The following example is a simple demonstration of the permutation array handle.

.. doxygenclass:: vtkm::cont::ArrayHandlePermutation
   :members:

.. load-example:: ArrayHandlePermutation
   :file: GuideExampleArrayHandlePermutation.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandlePermutation`.

The :file:`vtkm/cont/ArrayHandlePermutation.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandlePermutation` that takes instances of the index and value array handles and returns a permutation array.
This function can sometimes be used to avoid having to declare the full array type.

.. doxygenfunction:: vtkm::cont::make_ArrayHandlePermutation

.. load-example:: MakeArrayHandlePermutation
   :file: GuideExampleArrayHandlePermutation.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandlePermutation`.

.. commonerrors::
   When using an :class:`vtkm::cont::ArrayHandlePermutation`, take care that all the provided indices in the index array point to valid locations in the values array.
   Bad indices can cause reading from or writing to invalid memory locations, which can be difficult to debug.
   Also, be wary about having duplicate indices, which means that multiple array entries point to the same memory location.
   This will work fine when using the array as input, but will cause a dangerous race condition if used as an output.

.. didyouknow::
   You can write to a :class:`vtkm::cont::ArrayHandlePermutation` by, for example, using it as an output array.
   Writes to the :class:`vtkm::cont::ArrayHandlePermutation` will go to the respective location in the source array.
   However, :class:`vtkm::cont::ArrayHandlePermutation` cannot be resized.


------------------------------
Zipped Arrays
------------------------------

.. index::
   single: array handle; zipped
   single: zipped array handle

A zip array is a fancy array handle that combines two arrays of the same size to pair up the corresponding values.
Each element in the zipped array is a :class:`vtkm::Pair` containing the values of the two respective arrays.
These pairs are not stored in their own memory space.
Rather, the pairs are generated as the array is used.
Writing a pair to the zipped array writes the values in the two source arrays.

Specifying a zipped array in |VTKm| is straightforward.
|VTKm| has a class named :class:`vtkm::cont::ArrayHandleZip` that takes the two arrays providing values for the first and second entries in the pairs.
The following example is a simple demonstration of creating a zip array handle.

.. doxygenclass:: vtkm::cont::ArrayHandleZip
   :members:

.. load-example:: ArrayHandleZip
   :file: GuideExampleArrayHandleZip.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleZip`.

The :file:`vtkm/cont/ArrayHandleZip.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandleZip` that takes instances of the two array handles and returns a zip array.
This function can sometimes be used to avoid having to declare the full array type.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleZip

.. load-example:: MakeArrayHandleZip
   :file: GuideExampleArrayHandleZip.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleZip`.


------------------------------
Coordinate System Arrays
------------------------------

Many of the data structures we use in |VTKm| are described in a 3D coordinate system.
Although, as we will see in :chapref:`dataset:Data Sets`, we can use any :class:`vtkm::cont::ArrayHandle` to store point coordinates, including a raw array of 3D vectors, there are some common patterns for point coordinates that we can use specialized arrays to better represent the data.

.. index::
   double: array handle; uniform point coordinates

There are two fancy array handles that each handle a special form of coordinate system.
The first such array handle is :class:`vtkm::cont::ArrayHandleUniformPointCoordinates`, which represents a uniform sampling of space.
The constructor for :class:`vtkm::cont::ArrayHandleUniformPointCoordinates` takes three arguments.
The first argument is a :type:`vtkm::Id3` that specifies the number of samples in the :math:`x`, :math:`y`, and :math:`z` directions.
The second argument, which is optional, specifies the origin (the location of the first point at the lower left corner).
If not specified, the origin is set to :math:`[0,0,0]`.
The third argument, which is also optional, specifies the distance between samples in the :math:`x`, :math:`y`, and :math:`z` directions.
If not specified, the spacing is set to 1 in each direction.

.. doxygenclass:: vtkm::cont::ArrayHandleUniformPointCoordinates
   :members:

.. load-example:: ArrayHandleUniformPointCoordinates
   :file: GuideExampleArrayHandleCoordinateSystems.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleUniformPointCoordinates`.

.. index::
   double: array handle; Cartesian product
   double: array handle; rectilinear point coordinates

The second fancy array handle for special coordinate systems is :class:`vtkm::cont::ArrayHandleCartesianProduct`, which represents a rectilinear sampling of space where the samples are axis aligned but have variable spacing.
Sets of coordinates of this type are most efficiently represented by having a separate array for each component of the axis, and then for each :math:`[i,j,k]` index of the array take the value for each component from each array using the respective index.
This is equivalent to performing a Cartesian product on the arrays.

.. doxygenclass:: vtkm::cont::ArrayHandleCartesianProduct
   :members:

:class:`vtkm::cont::ArrayHandleCartesianProduct` is a templated class.
It has three template parameters, which are the types of the arrays used for the :math:`x`, :math:`y`, and :math:`z` axes.
The constructor for :class:`vtkm::cont::ArrayHandleCartesianProduct` takes the three arrays.

.. load-example:: ArrayHandleCartesianProduct
   :file: GuideExampleArrayHandleCoordinateSystems.cxx
   :caption: Using a :class:`vtkm::cont::ArrayHandleCartesianProduct`.

The :file:`vtkm/cont/ArrayHandleCartesianProduct.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandleCartesianProduct` that takes the three axis arrays and returns an array of the Cartesian product.
This function can sometimes be used to avoid having to declare the full array type.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleCartesianProduct

.. load-example:: MakeArrayHandleCartesianProduct
   :file: GuideExampleArrayHandleCoordinateSystems.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleCartesianProduct`.

.. didyouknow::
   These specialized arrays for coordinate systems greatly reduce the code duplication in |VTKm|.
   Most scientific visualization systems need separate implementations of algorithms for uniform, rectilinear, and unstructured grids.
   But in |VTKm| an algorithm can be written once and then applied to all these different grid structures by using these specialized array handles and letting the compiler's templates optimize the code.

.. didyouknow::
   The special array handles in this section are designed to represent point coordinates in particular, common configurations.
   However, the array for a :class:`vtkm::cont::CoordinateSystem` does not have to be one of these arrays.
   For example, it is common to use a :class:`vtkm::cont::ArrayHandleBasic` to represent points in general position.


------------------------------
Composite Vector Arrays
------------------------------

.. index::
   double: array handle; composite vector

A composite vector array is a fancy array handle that combines two to four arrays of the same size and value type and combines their corresponding values to form a :class:`vtkm::Vec`.
A composite vector array is similar in nature to a zipped array (described in :secref:`fancy-array-handles:Zipped Arrays`) except that values are combined into :class:`vtkm::Vec`'s instead of :class:`vtkm::Pair`'s.
The composite vector array is also similar to a structure of arrays (described in :secref:`memory-layout:Structure of Arrays`) except that any type of array handles can be used for the components rather than a basic array handle.
The created :class:`vtkm::Vec`'s are not stored in their own memory space.
Rather, the :class:`vtkm::Vec`'s are generated as the array is used.
Writing :class:`vtkm::Vec`'s to the composite vector array writes values into the components of the source arrays.

A composite vector array can be created using the :class:`vtkm::cont::ArrayHandleCompositeVector` class.
This class has a variadic template argument that is a "signature" for the arrays to be combined.
The constructor for :class:`vtkm::cont::ArrayHandleCompositeVector` takes instances of the array handles to combine.

.. doxygenclass:: vtkm::cont::ArrayHandleCompositeVector
   :members:

.. load-example:: ArrayHandleCompositeVectorBasic
   :file: GuideExampleArrayHandleCompositeVector.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleCompositeVector`.

The :file:`vtkm/cont/ArrayHandleCompositeVector.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandleCompositeVector` which takes a variable number of array handles and returns an :class:`vtkm::cont::ArrayHandleCompositeVector`.
This function can sometimes be used to avoid having to declare the full array type.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleCompositeVector

.. load-example:: MakeArrayHandleCompositeVector
   :file: GuideExampleArrayHandleCompositeVector.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleCompositeVector`.


------------------------------
Extract Component Arrays
------------------------------

.. index::
   double: array handle; extract component

Component extraction allows access to a single component of an :class:`vtkm::cont::ArrayHandle` with a :class:`vtkm::Vec` as the :member:`vtkm::cont::ArrayHandle::ValueType`.
:class:`vtkm::cont::ArrayHandleExtractComponent` allows one component of a vector array to be extracted without creating a copy of the data.
:class:`vtkm::cont::ArrayHandleExtractComponent` can also be combined with :class:`vtkm::cont::ArrayHandleCompositeVector` (described in :secref:`fancy-array-handles:Composite Vector Arrays`) to arbitrarily stitch several components from multiple arrays together.

.. doxygenclass:: vtkm::cont::ArrayHandleExtractComponent
   :members:

As a simple example, consider an :class:`vtkm::cont::ArrayHandle` containing 3D coordinates for a collection of points and a filter that only operates on the points' elevations (Z, in this example).
We can easily create the elevation array on-the-fly without allocating a new array as in the following example.

.. load-example:: ArrayHandleExtractComponent
   :file: GuideExampleArrayHandleExtractComponent.cxx
   :caption: Extracting components of :class:`vtkm::Vec`'s in an array with :class:`vtkm::cont::ArrayHandleExtractComponent`.

The :file:`vtkm/cont/ArrayHandleExtractComponent.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandleExtractComponent` that takes an :class:`vtkm::cont::ArrayHandle` of :class:`vtkm::Vec`'s and :type:`vtkm::IdComponent` which returns an appropriately typed :class:`vtkm::cont::ArrayHandleExtractComponent` containing the values for a specified component.
The index of the component to extract is provided as an argument to :func:`vtkm::cont::make_ArrayHandleExtractComponent`, which is required.
The use of :func:`vtkm::cont::make_ArrayHandleExtractComponent` can be used to avoid having to declare the full array type.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleExtractComponent

.. load-example:: MakeArrayHandleExtractComponent
   :file: GuideExampleArrayHandleExtractComponent.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleExtractComponent`.

.. didyouknow::
   An alternate way to extract a component from a :class:`vtkm::cont::ArrayHandle` is to use the :func:`vtkm::cont::ArrayExtractComponent` function.
   Rather than wrap a :class:`vtkm::cont::ArrayHandleExtractComponent` around the target array, it converts the array into a :class:`vtkm::cont::ArrayHandleStride`, as described in :secref:`memory-layout:Strided Arrays`.
   This can be advantageous when trying to unify the storage type of different array types, but can work poorly for some array types.


------------------------------
Swizzle Arrays
------------------------------

.. index::
   double: array handle; swizzle

It is often useful to reorder or remove specific components from an :class:`vtkm::cont::ArrayHandle` with a :class:`vtkm::Vec` :member:`vtkm::cont::ArrayHandle::ValueType`.
:class:`vtkm::cont::ArrayHandleSwizzle` provides an easy way to accomplish this.

The constructor of :class:`vtkm::cont::ArrayHandleSwizzle` specifies a "component map," which defines the swizzle operation.
This map consists of the components from the input :class:`vtkm::cont::ArrayHandle`, which will be exposed in the :class:`vtkm::cont::ArrayHandleSwizzle`.
For instance, constructing ``vtkm::cont::ArrayHandleSwizzle<Some3DArrayType, 3>`` with ``vtkm::IdComponent3(0, 2, 1)`` as the second constructor argument will allow access to a 3D array, but with the Y and Z components exchanged.
This rearrangement does not create a copy, and occurs on-the-fly as data are accessed through the :class:`vtkm::cont::ArrayHandleSwizzle`'s portal.
This fancy array handle can also be used to eliminate unnecessary components from an :class:`vtkm::cont::ArrayHandle`'s data, as shown below.

.. doxygenclass:: vtkm::cont::ArrayHandleSwizzle
   :members:

.. load-example:: ArrayHandleSwizzle
   :file: GuideExampleArrayHandleSwizzle.cxx
   :caption: Swizzling components of :class:`vtkm::Vec`'s in an array with :class:`vtkm::cont::ArrayHandleSwizzle`.

The :file:`vtkm/cont/ArrayHandleSwizzle.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandleSwizzle` that takes an :class:`vtkm::cont::ArrayHandle` of :class:`vtkm::Vec`'s and returns an appropriately typed :class:`vtkm::cont::ArrayHandleSwizzle` containing swizzled vectors.
The use of :func:`vtkm::cont::make_ArrayHandleSwizzle` can be used to avoid having to declare the full array type.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleSwizzle(const ArrayHandleType&, const vtkm::Vec<vtkm::IdComponent, OutSize>&)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleSwizzle(const ArrayHandleType&, vtkm::IdComponent, SwizzleIndexTypes...)

.. load-example:: MakeArrayHandleSwizzle
   :file: GuideExampleArrayHandleSwizzle.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleSwizzle`.


------------------------------
Grouped Vector Arrays
------------------------------

.. index::
   double: array handle; group vector

A grouped vector array is a fancy array handle that groups consecutive values of an array together to form a :class:`vtkm::Vec`.
The source array must be of a length that is divisible by the requested :class:`vtkm::Vec` size.
The created :class:`vtkm::Vec`'s are not stored in their own memory space.
Rather, the :class:`vtkm::Vec`'s are generated as the array is used.
Writing :class:`vtkm::Vec`'s to the grouped vector array writes values into the the source array.

A grouped vector array is created using the :class:`vtkm::cont::ArrayHandleGroupVec` class.
This templated class has two template arguments.
The first argument is the type of array being grouped and the second argument is an integer specifying the size of the :class:`vtkm::Vec`'s to create (the number of values to group together).

.. doxygenclass:: vtkm::cont::ArrayHandleGroupVec
   :members:

.. load-example:: ArrayHandleGroupVecBasic
   :file: GuideExampleArrayHandleGroupVec.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleGroupVec`.

The :file:`vtkm/cont/ArrayHandleGroupVec.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandleGroupVec` that takes an instance of the array to group into :class:`vtkm::Vec`'s.
You must specify the size of the :class:`vtkm::Vec`'s as a template parameter when using :func:`vtkm::cont::make_ArrayHandleGroupVec`.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleGroupVec

.. load-example:: MakeArrayHandleGroupVec
   :file: GuideExampleArrayHandleGroupVec.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleGroupVec`.

.. didyouknow::
   :class:`vtkm::cont::ArrayHandleGroupVec` requires you to specify the number of components at compile time.
   For a similar functionality, consider using :class:`vtkm::cont::ArrayHandleRuntimeVec`, described in :secref:`memory-layout:Runtime Vec Arrays`.
   This allows the runtime selection of :class:`vtkm::Vec` sizes and can be applied to existing basic arrays, but is limited in other ways.

:class:`vtkm::cont::ArrayHandleGroupVec` is handy when you need to build an array of vectors that are all of the same length, but what about when you need an array of vectors of different lengths?
One common use case for this is if you are defining a collection of polygons of different sizes (triangles, quadrilaterals, pentagons, and so on).
We would like to define an array such that the data for each polygon were stored in its own :class:`vtkm::Vec` (or, rather, |Veclike|) object.
:class:`vtkm::cont::ArrayHandleGroupVecVariable` does just that.

:class:`vtkm::cont::ArrayHandleGroupVecVariable` takes two arrays. The first array, identified as the "source" array, is a flat representation of the values (much like the array used with :class:`vtkm::cont::ArrayHandleGroupVec`).
The second array, identified as the "offsets" array, provides for each vector the index into the source array where the start of the vector is.
The offsets array must be monotonically increasing.
The size of the offsets array is one greater than the number of vectors in the resulting array.
The first offset is always 0 and the last offset is always the size of the input source array.
The first and second template parameters to :class:`vtkm::cont::ArrayHandleGroupVecVariable` are the types for the source and offset arrays, respectively.

.. doxygenclass:: vtkm::cont::ArrayHandleGroupVecVariable
   :members:

It is often the case that you will start with a group of vector lengths rather than offsets into the source array.
If this is the case, then the :func:`vtkm::cont::ConvertNumComponentsToOffsets` helper function can convert an array of vector lengths to an array of offsets.
The first argument to this function is always the array of vector lengths.
The second argument, which is optional, is a reference to a :class:`vtkm::cont::ArrayHandle` into which the offsets should be stored.
If this offset array is not specified, an :class:`vtkm::cont::ArrayHandle` will be returned from the function instead.
The third argument, which is also optional, is a reference to a :type:`vtkm::Id` into which the expected size of the source array is put.
Having the size of the source array is often helpful, as it can be used to allocate data for the source array or check the source array's size.
It is also OK to give the expected size reference but not the offset array reference.

.. doxygenfunction:: vtkm::cont::ConvertNumComponentsToOffsets(const vtkm::cont::UnknownArrayHandle&, vtkm::cont::ArrayHandle<vtkm::Id>&, vtkm::Id&, vtkm::cont::DeviceAdapterId)

.. load-example:: ArrayHandleGroupVecVariable
   :file: GuideExampleArrayHandleGroupVec.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleGroupVecVariable`.

The :file:`vtkm/cont/ArrayHandleGroupVecVariable.h` header also contains the templated convenience function :func:`vtkm::cont::make_ArrayHandleGroupVecVariable` that takes an instance of the source array to group into |Veclike| objects and the offset array.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleGroupVecVariable

.. load-example:: MakeArrayHandleGroupVecVariable
   :file: GuideExampleArrayHandleGroupVec.cxx
   :caption: Using :func:`vtkm::cont::make_ArrayHandleGroupVecVariable`.

.. didyouknow::
   You can write to :class:`vtkm::cont::ArrayHandleGroupVec` and :class:`vtkm::cont::ArrayHandleGroupVecVariable` by, for example, using it as an output array.
   Writes to these arrays will go to the respective location in the source array.
   :class:`vtkm::cont::ArrayHandleGroupVec` can also be allocated and resized (which in turn causes the source array to be allocated).
   However, :class:`vtkm::cont::ArrayHandleGroupVecVariable` cannot be resized and the source array must be pre-allocated.
   You can use the source array size value returned from :func:`vtkm::cont::ConvertNumComponentsToOffsets` to allocate source arrays.

.. commonerrors::
   Keep in mind that the values stored in a :class:`vtkm::cont::ArrayHandleGroupVecVariable` are not actually :class:`vtkm::Vec` objects.
   Rather, they are "|Veclike|" objects, which has some subtle but important ramifications.
   First, the type will not match the :class:`vtkm::Vec` template, and there is no automatic conversion to :class:`vtkm::Vec` objects.
   Thus, many functions that accept :class:`vtkm::Vec` objects as parameters will not accept the |Veclike| object.
   Second, the size of |Veclike| objects are not known until runtime.
   See :secref:`base-types:Vector Types` and :secref:`advanced-types:Vector Traits` for more information on the difference between :class:`vtkm::Vec` and |Veclike| objects.


------------------------------
Random Arrays
------------------------------

.. index::
   double: array handle; random

The basis of generating random numbers in |VTKm| is built on the :class:`vtkm::cont::ArrayHandleRandomUniformBits`.
An uniform random bits array is a fancy array handle that generates pseudo random bits as :type:`vtkm::Unit64` in its entries.
The uniform random bits array provides this array without actually using any memory.

.. doxygenclass:: vtkm::cont::ArrayHandleRandomUniformBits
   :members:

The constructor for :class:`vtkm::cont::ArrayHandleRandomUniformBits` takes two arguments: the first argument is the length of the array handle, the second is a seed of type ``vtkm::Vec<Uint32, 1>``.
If the seed is not specified, the C++11 ``std::random_device`` is used as default.

.. load-example:: ArrayHandleRandomUniformBits
   :file: GuideExampleArrayHandleRandom.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleRandomUniformBits`.

:class:`vtkm::cont::ArrayHandleRandomUniformBits` is functional, in the sense that once an instance of :class:`vtkm::cont::ArrayHandleRandomUniformBits` is created, its content does not change and always returns the same :type:`vtkm::UInt64` value given the same index.

.. load-example:: ArrayHandleRandomUniformBitsFunctional
   :file: GuideExampleArrayHandleRandom.cxx
   :caption: :class:`vtkm::cont::ArrayHandleRandomUniformBits` is functional.

To generate a new set of random bits, we need to create another instance of :class:`vtkm::cont::ArrayHandleRandomUniformBits` with a different seed, we can either let ``std::random_device`` provide a unique seed or use some unique identifier such as iteration number as the seed.

.. load-example:: ArrayHandleRandomUniformBitsIteration
   :file: GuideExampleArrayHandleRandom.cxx
   :caption: Independent :class:`vtkm::cont::ArrayHandleRandomUniformBits`.

The random bits provided by :class:`vtkm::cont::ArrayHandleRandomUniformBits` can be manipulated to provide random numbers with specific distributions.
|VTKm| provides some specialized classes that implement common distributions.

The :class:`vtkm::cont::ArrayHandleRandomUniformReal` class generates an array of numbers sampled from a real uniform distribution in the range :math:`[0, 1)`.

.. doxygenclass:: vtkm::cont::ArrayHandleRandomUniformReal
   :members:

.. load-example:: ArrayHandleRandomUniformReal
   :file: GuideExampleArrayHandleRandom.cxx
   :caption: Generating a random cloud of point coordinates in the box bounded by [0, 1].

The :class:`vtkm::cont::ArrayHandleRandomStandardNormal` class generates an array of numbers sampled from a standard normal distribution.
This provides a set of points centered at 0 and with probability exponentially diminishing away from 0 in both the positive and negative directions.

.. doxygenclass:: vtkm::cont::ArrayHandleRandomStandardNormal
   :members:

.. load-example:: ArrayHandleRandomStandardNormal
   :file: GuideExampleArrayHandleRandom.cxx
   :caption: Generating a random cloud of point coordinates in a Gaussian distribution centered at the origin.

.. didyouknow::
   The distributions of the provided random array handles can manipulated by shifting and scaling the values they provide.
   This will keep the general distribution shape but change the range.
   This manipulation can happen in a worklet from the values returned from the arrays or they can be generated automatically by wrapping the random arrays in a :class:`vtkm::cont::ArrayHandleTransform`.

.. todo:: Add a reference to the section describing :class:`vtkm::cont::ArrayHandleTransform`.
