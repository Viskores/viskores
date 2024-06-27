==============================
Memory Layout of Array Handles
==============================

.. index:: array handle; memory layout

:chapref:`basic-array-handles:Basic Array Handles` describes the basics of the :class:`vtkm::cont::ArrayHandle` class, which is the interface to the arrays of data that |VTKm| operates on.
Recall that :class:`vtkm::cont::ArrayHandle` is a templated class with two template parameters.
The first template argument is the type of each item in the array.
The second parameter, which is optional, determines how the array is stored in memory.
This can be used in a variety of different ways, but its primary purpose is to provide a strategy for laying the data out in memory.
This chapter documents the ways in which |VTKm| can store and access arrays of data in different layouts.

------------------------------
Basic Memory Layout
------------------------------

.. index::
   single: array handle; basic
   single: basic array handle

If the second storage template parameter of :class:`vtkm::cont::ArrayHandle` is not specified, it defaults to the basic memory layout.
This is roughly synonymous with a wrapper around a standard C array, much like ``std::vector``.
In fact, :secref:`basic-array-handles:Creating Array Handles` provides examples of wrapping a default :class:`vtkm::cont::ArrayHandle` around either a basic C array or a ``std::vector``.

|VTKm| provides :class:`vtkm::cont::ArrayHandleBasic` as a convenience class for working with basic array handles.
:class:`vtkm::cont::ArrayHandleBasic` is a simple subclass of :class:`vtkm::cont::ArrayHandle` with the default storage in the second template argument (which is :class:`vtkm::cont::StorageTagBasic`).
:class:`vtkm::cont::ArrayHandleBasic` and its superclass can be used more or less interchangeably.

.. doxygenclass:: vtkm::cont::ArrayHandleBasic
   :members:

Because a :class:`vtkm::cont::ArrayHandleBasic` represents arrays as a standard C array, it is possible to get a pointer to this array using either :func:`vtkm::cont::ArrayHandleBasic::GetReadPointer` or :func:`vtkm::cont::ArrayHandleBasic::GetWritePointer`.

.. load-example:: GetArrayPointer
   :file: GuideExampleArrayHandle.cxx
   :caption: Getting a standard C array from a basic array handle.

.. didyouknow::
   When you get an array pointer this way, the :class:`vtkm::cont::ArrayHandle` still has a reference to it.
   If using multiple threads, you can use a :class:`vtkm::cont::Token` object to lock the array.
   When the token is used to get a pointer, it will lock the array as long as the token exists.
   :numref:`ex:GetArrayPointer` demonstrates using a :class:`vtkm::cont::Token`.

--------------------
Structure of Arrays
--------------------

.. index::
   single: AOS
   single: SOA

The basic :class:`vtkm::cont::ArrayHandle` stores :class:`vtkm::Vec` objects in sequence.
In this sense, a basic array is an *Array of Structures* (AOS).
Another approach is to store each component of the structure (i.e., the :class:`vtkm::Vec`) in a separate array.
This is known as a *Structure of Arrays* (SOA).
There are advantages to this approach including potentially better cache performance and the ability to combine arrays already represented as separate components without copying them.
Arrays of this nature are represented with a :class:`vtkm::cont::ArrayHandleSOA`, which is a subclass of :class:`vtkm::cont::StorageTagSOA`.

.. doxygenclass:: vtkm::cont::ArrayHandleSOA
   :members:

:class:`vtkm::cont::ArrayHandleSOA` can be constructed and allocated just as a basic array handle.
Additionally, you can use its constructors or the :func:`vtkm::cont::make_ArrayHandleSOA` functions to build a :class:`vtkm::cont::ArrayHandleSOA` from basic :class:`vtkm::cont::ArrayHandle`'s that hold the components.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleSOA(std::initializer_list<vtkm::cont::ArrayHandle<typename vtkm::VecTraits<ValueType>::ComponentType, vtkm::cont::StorageTagBasic>> &&)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleSOA(const vtkm::cont::ArrayHandle<ComponentType, vtkm::cont::StorageTagBasic>&, const RemainingArrays&...)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleSOA(std::initializer_list<std::vector<typename vtkm::VecTraits<ValueType>::ComponentType>>&&)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleSOA(vtkm::CopyFlag, const std::vector<ComponentType>&, RemainingVectors&&...)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleSOA(vtkm::CopyFlag, std::vector<ComponentType>&&, RemainingVectors&&...)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleSOAMove(std::vector<ComponentType>&&, RemainingVectors&&...)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleSOA(std::initializer_list<const typename vtkm::VecTraits<ValueType>::ComponentType*>&&, vtkm::Id, vtkm::CopyFlag)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleSOA(vtkm::Id, vtkm::CopyFlag, const ComponentType*, const RemainingArrays*...)

.. load-example:: ArrayHandleSOAFromComponentArrays
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an SOA array handle from component arrays.

.. didyouknow::
   In addition to constructing a :class:`vtkm::cont::ArrayHandleSOA` from its component arrays, you can get the component arrays back out using the :func:`vtkm::cont::ArrayHandleSOA::GetArray` method.

--------------------
Strided Arrays
--------------------

.. index::
   double: array handle; stride
   double: array handle; offset
   double: array handle; modulo
   double: array handle; divisor

:class:`vtkm::cont::ArrayHandleBasic` operates on a tightly packed array.
That is, each value follows immediately after the proceeding value in memory.
However, it is often convenient to access values at different strides or offsets.
This allows representations of data that are not tightly packed in memory.
The :class:`vtkm::cont::ArrayHandleStride` class allows arrays with different data packing.

.. doxygenclass:: vtkm::cont::ArrayHandleStride
   :members:

The most common use of :class:`vtkm::cont::ArrayHandleStride` is to pull components out of arrays.
:class:`vtkm::cont::ArrayHandleStride` is seldom constructed directly.
Rather, |VTKm| has mechanisms to extract a component from an array.
To extract a component directly from a :class:`vtkm::cont::ArrayHandle`, use :func:`vtkm::cont::ArrayExtractComponent`.

.. doxygenfunction:: vtkm::cont::ArrayExtractComponent

The main advantage of extracting components this way is to convert data represented in different types of arrays into an array of a single type.
For example, :class:`vtkm::cont::ArrayHandleStride` can represent a component from either a :class:`vtkm::cont::ArrayHandleBasic` or a :class:`vtkm::cont::ArrayHandleSOA` by just using different stride values.
This is used by :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` and elsewhere to create a concrete array handle class without knowing the actual class.

.. commonerrors::
   Many, but not all, of |VTKm|'s arrays can be represented by a :class:`vtkm::cont::ArrayHandleStride` directly without copying.
   If |VTKm| cannot easily create a :class:`vtkm::cont::ArrayHandleStride` when attempting such an operation, it will use a slow copying fallback.
   A warning will be issued whenever this happens.
   Be on the lookout for such warnings and consider changing the data representation when that happens.

--------------------
Runtime Vec Arrays
--------------------

Because many of the devices |VTKm| runs on cannot efficiently allocate memory while an algorithm is running, the data held in :class:`vtkm::cont::ArrayHandle`'s are usually required to be a static size.
For example, the :class:`vtkm::Vec` object often used as the value type for :class:`vtkm::cont::ArrayHandle` has a number of components that must be defined at compile time.

This is a problem in cases where the size of a vector object cannot be determined at compile time.
One class to help alleviate this problem is :class:`vtkm::cont::ArrayHandleRuntimeVec`.
This array handle stores data in the same way as :class:`vtkm::cont::ArrayHandleBasic` with a :class:`vtkm::Vec` value type, but the size of the ``Vec`` can be set at runtime.

.. doxygenclass:: vtkm::cont::ArrayHandleRuntimeVec
   :members:

A :class:`vtkm::cont::ArrayHandleRuntimeVec` is easily created from existing data using one of the :func:`vtkm::cont::make_ArrayHandleRuntimeVec` functions.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleRuntimeVec(vtkm::IdComponent, const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>&)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleRuntimeVec(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>&)

|VTKm| also provides several convenience functions to convert a basic C array or ``std::vector`` to a :class:`vtkm::cont::ArrayHandleRuntimeVec`.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleRuntimeVec(vtkm::IdComponent, const T*, vtkm::Id, vtkm::CopyFlag)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleRuntimeVecMove(vtkm::IdComponent, T*&, vtkm::Id, vtkm::cont::internal::BufferInfo::Deleter, vtkm::cont::internal::BufferInfo::Reallocater)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleRuntimeVec(vtkm::IdComponent, const std::vector<T, Allocator>&, vtkm::CopyFlag)
.. doxygenfunction:: vtkm::cont::make_ArrayHandleRuntimeVecMove(vtkm::IdComponent, std::vector<T, Allocator>&&)

The advantage of this class is that a :class:`vtkm::cont::ArrayHandleRuntimeVec` can be created in a routine that does not know the number of components at runtime and then later retrieved as a basic :class:`vtkm::cont::ArrayHandle` with a :class:`vtkm::Vec` of the correct size.
This often consists of a file reader or other data ingestion creating :class:`vtkm::cont::ArrayHandleRuntimeVec` objects and storing them in :class:`vtkm::cont::UnknownArrayHandle`, which is used as an array container for :class:`vtkm::cont::DataSet`.
Filters that then subsequently operate on the :class:`vtkm::cont::DataSet` can retrieve the data as a :class:`vtkm::cont::ArrayHandle` of the appropriate :class:`vtkm::Vec` size.

.. load-example:: GroupWithRuntimeVec
   :file: GuideExampleArrayHandleRuntimeVec.cxx
   :caption: Loading a data with runtime component size and using with a static sized filter.

.. didyouknow::
   Wrapping a basic array in a :class:`vtkm::cont::ArrayHandleRuntimeVec` has a similar effect as wrapping the array in a :class:`vtkm::cont::ArrayHandleGroupVec`.
   The difference is in the context in which they are used.
   If the size of the ``Vec`` is known at compile time *and* the array is going to immediately be used (such as operated on by a worklet), then :class:`vtkm::cont::ArrayHandleGroupVec` should be used.
   However, if the ``Vec`` size is not known or the array will be stored in an object like :class:`vtkm::cont::UnknownArrayHandle`, then :class:`vtkm::cont::ArrayHandleRuntimeVec` is a better choice.

It is also possible to get a :class:`vtkm::cont::ArrayHandleRuntimeVec` from a :class:`vtkm::cont::UnknownArrayHandle` that was originally stored as a basic array.
This is convenient for operations that want to operate on arrays with an unknown ``Vec`` size.

.. load-example:: GetRuntimeVec
   :file: GuideExampleArrayHandleRuntimeVec.cxx
   :caption: Using :class:`vtkm::cont::ArrayHandleRuntimeVec` to get an array regardless of the size of the contained :class:`vtkm::Vec` values.


---------------------------------------------
Recombined Vec Arrays of Strided Components
---------------------------------------------

|VTKm| contains a special array, :class:`vtkm::cont::ArrayHandleRecombineVec`, to combine component arrays represented in :class:`vtkm::cont::ArrayHandleStride` together to form `Vec` values.
:class:`vtkm::cont::ArrayHandleRecombineVec` is similar to :class:`vtkm::cont::ArrayHandleSOA` (see :secref:`memory-layout:Structure of Arrays`) except that (1) it holds stride arrays for its components instead of basic arrays and that (2) the number of components can be specified at runtime.
:class:`vtkm::cont::ArrayHandleRecombineVec` is mainly provided for the implementation of extracting arrays out of a :class:`vtkm::cont::UnknownArrayHandle` (see :secref:`unknown-array-handle:Extracting All Components`).

.. doxygenclass:: vtkm::cont::ArrayHandleRecombineVec
