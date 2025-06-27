==============================
Memory Layout of Array Handles
==============================

.. index:: array handle; memory layout

:chapref:`basic-array-handles:Basic Array Handles` describes the basics of the :class:`viskores::cont::ArrayHandle` class, which is the interface to the arrays of data that |Viskores| operates on.
Recall that :class:`viskores::cont::ArrayHandle` is a templated class with two template parameters.
The first template argument is the type of each item in the array.
The second parameter, which is optional, determines how the array is stored in memory.
This can be used in a variety of different ways, but its primary purpose is to provide a strategy for laying the data out in memory.
This chapter documents the ways in which |Viskores| can store and access arrays of data in different layouts.

------------------------------
Basic Memory Layout
------------------------------

.. index::
   single: array handle; basic
   single: basic array handle

If the second storage template parameter of :class:`viskores::cont::ArrayHandle` is not specified, it defaults to the basic memory layout.
This is roughly synonymous with a wrapper around a standard C array, much like ``std::vector``.
In fact, :secref:`basic-array-handles:Creating Array Handles` provides examples of wrapping a default :class:`viskores::cont::ArrayHandle` around either a basic C array or a ``std::vector``.

|Viskores| provides :class:`viskores::cont::ArrayHandleBasic` as a convenience class for working with basic array handles.
:class:`viskores::cont::ArrayHandleBasic` is a simple subclass of :class:`viskores::cont::ArrayHandle` with the default storage in the second template argument (which is :class:`viskores::cont::StorageTagBasic`).
:class:`viskores::cont::ArrayHandleBasic` and its superclass can be used more or less interchangeably.

.. doxygenclass:: viskores::cont::ArrayHandleBasic
   :members:

Because a :class:`viskores::cont::ArrayHandleBasic` represents arrays as a standard C array, it is possible to get a pointer to this array using either :func:`viskores::cont::ArrayHandleBasic::GetReadPointer` or :func:`viskores::cont::ArrayHandleBasic::GetWritePointer`.

.. load-example:: GetArrayPointer
   :file: GuideExampleArrayHandle.cxx
   :caption: Getting a standard C array from a basic array handle.

.. didyouknow::
   When you get an array pointer this way, the :class:`viskores::cont::ArrayHandle` still has a reference to it.
   If using multiple threads, you can use a :class:`viskores::cont::Token` object to lock the array.
   When the token is used to get a pointer, it will lock the array as long as the token exists.
   :numref:`ex:GetArrayPointer` demonstrates using a :class:`viskores::cont::Token`.

--------------------
Structure of Arrays
--------------------

.. index::
   single: AOS
   single: SOA

The basic :class:`viskores::cont::ArrayHandle` stores :class:`viskores::Vec` objects in sequence.
In this sense, a basic array is an *Array of Structures* (AOS).
Another approach is to store each component of the structure (i.e., the :class:`viskores::Vec`) in a separate array.
This is known as a *Structure of Arrays* (SOA).
There are advantages to this approach including potentially better cache performance and the ability to combine arrays already represented as separate components without copying them.
Arrays of this nature are represented with a :class:`viskores::cont::ArrayHandleSOA`, which is a subclass of :class:`viskores::cont::StorageTagSOA`.

.. doxygenclass:: viskores::cont::ArrayHandleSOA
   :members:

:class:`viskores::cont::ArrayHandleSOA` can be constructed and allocated just as a basic array handle.
Additionally, you can use its constructors or the :func:`viskores::cont::make_ArrayHandleSOA` functions to build a :class:`viskores::cont::ArrayHandleSOA` from basic :class:`viskores::cont::ArrayHandle`'s that hold the components.

.. doxygenfunction:: viskores::cont::make_ArrayHandleSOA(std::initializer_list<viskores::cont::ArrayHandle<typename viskores::VecTraits<ValueType>::ComponentType, viskores::cont::StorageTagBasic>> &&)
.. doxygenfunction:: viskores::cont::make_ArrayHandleSOA(const viskores::cont::ArrayHandle<ComponentType, viskores::cont::StorageTagBasic>&, const RemainingArrays&...)
.. doxygenfunction:: viskores::cont::make_ArrayHandleSOA(std::initializer_list<std::vector<typename viskores::VecTraits<ValueType>::ComponentType>>&&)
.. doxygenfunction:: viskores::cont::make_ArrayHandleSOA(viskores::CopyFlag, const std::vector<ComponentType>&, RemainingVectors&&...)
.. doxygenfunction:: viskores::cont::make_ArrayHandleSOA(viskores::CopyFlag, std::vector<ComponentType>&&, RemainingVectors&&...)
.. doxygenfunction:: viskores::cont::make_ArrayHandleSOAMove(std::vector<ComponentType>&&, RemainingVectors&&...)
.. doxygenfunction:: viskores::cont::make_ArrayHandleSOA(std::initializer_list<const typename viskores::VecTraits<ValueType>::ComponentType*>&&, viskores::Id, viskores::CopyFlag)
.. doxygenfunction:: viskores::cont::make_ArrayHandleSOA(viskores::Id, viskores::CopyFlag, const ComponentType*, const RemainingArrays*...)

.. load-example:: ArrayHandleSOAFromComponentArrays
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an SOA array handle from component arrays.

.. didyouknow::
   In addition to constructing a :class:`viskores::cont::ArrayHandleSOA` from its component arrays, you can get the component arrays back out using the :func:`viskores::cont::ArrayHandleSOA::GetArray` method.

--------------------
Strided Arrays
--------------------

.. index::
   double: array handle; stride
   double: array handle; offset
   double: array handle; modulo
   double: array handle; divisor

:class:`viskores::cont::ArrayHandleBasic` operates on a tightly packed array.
That is, each value follows immediately after the proceeding value in memory.
However, it is often convenient to access values at different strides or offsets.
This allows representations of data that are not tightly packed in memory.
The :class:`viskores::cont::ArrayHandleStride` class allows arrays with different data packing.

.. doxygenclass:: viskores::cont::ArrayHandleStride
   :members:

The most common use of :class:`viskores::cont::ArrayHandleStride` is to pull components out of arrays.
:class:`viskores::cont::ArrayHandleStride` is seldom constructed directly.
Rather, |Viskores| has mechanisms to extract a component from an array.
To extract a component directly from a :class:`viskores::cont::ArrayHandle`, use :func:`viskores::cont::ArrayExtractComponent`.

.. doxygenfunction:: viskores::cont::ArrayExtractComponent

The main advantage of extracting components this way is to convert data represented in different types of arrays into an array of a single type.
For example, :class:`viskores::cont::ArrayHandleStride` can represent a component from either a :class:`viskores::cont::ArrayHandleBasic` or a :class:`viskores::cont::ArrayHandleSOA` by just using different stride values.
This is used by :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` and elsewhere to create a concrete array handle class without knowing the actual class.

.. commonerrors::
   Many, but not all, of |Viskores|'s arrays can be represented by a :class:`viskores::cont::ArrayHandleStride` directly without copying.
   If |Viskores| cannot easily create a :class:`viskores::cont::ArrayHandleStride` when attempting such an operation, it will use a slow copying fallback.
   A warning will be issued whenever this happens.
   Be on the lookout for such warnings and consider changing the data representation when that happens.

--------------------
Runtime Vec Arrays
--------------------

Because many of the devices |Viskores| runs on cannot efficiently allocate memory while an algorithm is running, the data held in :class:`viskores::cont::ArrayHandle`'s are usually required to be a static size.
For example, the :class:`viskores::Vec` object often used as the value type for :class:`viskores::cont::ArrayHandle` has a number of components that must be defined at compile time.

This is a problem in cases where the size of a vector object cannot be determined at compile time.
One class to help alleviate this problem is :class:`viskores::cont::ArrayHandleRuntimeVec`.
This array handle stores data in the same way as :class:`viskores::cont::ArrayHandleBasic` with a :class:`viskores::Vec` value type, but the size of the ``Vec`` can be set at runtime.

.. doxygenclass:: viskores::cont::ArrayHandleRuntimeVec
   :members:

A :class:`viskores::cont::ArrayHandleRuntimeVec` is easily created from existing data using one of the :func:`viskores::cont::make_ArrayHandleRuntimeVec` functions.

.. doxygenfunction:: viskores::cont::make_ArrayHandleRuntimeVec(viskores::IdComponent, const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>&)
.. doxygenfunction:: viskores::cont::make_ArrayHandleRuntimeVec(const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>&)

|Viskores| also provides several convenience functions to convert a basic C array or ``std::vector`` to a :class:`viskores::cont::ArrayHandleRuntimeVec`.

.. doxygenfunction:: viskores::cont::make_ArrayHandleRuntimeVec(viskores::IdComponent, const T*, viskores::Id, viskores::CopyFlag)
.. doxygenfunction:: viskores::cont::make_ArrayHandleRuntimeVecMove(viskores::IdComponent, T*&, viskores::Id, viskores::cont::internal::BufferInfo::Deleter, viskores::cont::internal::BufferInfo::Reallocater)
.. doxygenfunction:: viskores::cont::make_ArrayHandleRuntimeVec(viskores::IdComponent, const std::vector<T, Allocator>&, viskores::CopyFlag)
.. doxygenfunction:: viskores::cont::make_ArrayHandleRuntimeVecMove(viskores::IdComponent, std::vector<T, Allocator>&&)

The advantage of this class is that a :class:`viskores::cont::ArrayHandleRuntimeVec` can be created in a routine that does not know the number of components at runtime and then later retrieved as a basic :class:`viskores::cont::ArrayHandle` with a :class:`viskores::Vec` of the correct size.
This often consists of a file reader or other data ingestion creating :class:`viskores::cont::ArrayHandleRuntimeVec` objects and storing them in :class:`viskores::cont::UnknownArrayHandle`, which is used as an array container for :class:`viskores::cont::DataSet`.
Filters that then subsequently operate on the :class:`viskores::cont::DataSet` can retrieve the data as a :class:`viskores::cont::ArrayHandle` of the appropriate :class:`viskores::Vec` size.

.. load-example:: GroupWithRuntimeVec
   :file: GuideExampleArrayHandleRuntimeVec.cxx
   :caption: Loading a data with runtime component size and using with a static sized filter.

.. didyouknow::
   Wrapping a basic array in a :class:`viskores::cont::ArrayHandleRuntimeVec` has a similar effect as wrapping the array in a :class:`viskores::cont::ArrayHandleGroupVec`.
   The difference is in the context in which they are used.
   If the size of the ``Vec`` is known at compile time *and* the array is going to immediately be used (such as operated on by a worklet), then :class:`viskores::cont::ArrayHandleGroupVec` should be used.
   However, if the ``Vec`` size is not known or the array will be stored in an object like :class:`viskores::cont::UnknownArrayHandle`, then :class:`viskores::cont::ArrayHandleRuntimeVec` is a better choice.

It is also possible to get a :class:`viskores::cont::ArrayHandleRuntimeVec` from a :class:`viskores::cont::UnknownArrayHandle` that was originally stored as a basic array.
This is convenient for operations that want to operate on arrays with an unknown ``Vec`` size.

.. load-example:: GetRuntimeVec
   :file: GuideExampleArrayHandleRuntimeVec.cxx
   :caption: Using :class:`viskores::cont::ArrayHandleRuntimeVec` to get an array regardless of the size of the contained :class:`viskores::Vec` values.


---------------------------------------------
Recombined Vec Arrays of Strided Components
---------------------------------------------

|Viskores| contains a special array, :class:`viskores::cont::ArrayHandleSOAStride`, to combine component arrays represented in :class:`viskores::cont::ArrayHandleStride` together to form `Vec` values.
:class:`viskores::cont::ArrayHandleSOAStride` is similar to :class:`viskores::cont::ArrayHandleSOA` (see :secref:`memory-layout:Structure of Arrays`) except that it holds stride arrays for its components instead of basic arrays.
:class:`viskores::cont::ArrayHandleSOAStride` is mainly provided for the implementation of extracting arrays out of a :class:`viskores::cont::UnknownArrayHandle` (see :secref:`unknown-array-handle:Extracting a Known Value Type from Unknown Storage`).

.. doxygenclass:: viskores::cont::ArrayHandleSOAStride
   :members:

|Viskores| also contains a similar special array names :class:`viskores::cont::ArrayHandleRecombineVec`.
This array is similar to :class:`viskores::cont::ArrayHandleSOAStride` except that
the number of components can be specified at runtime.
This is useful when you know little or nothing about the value type and storage type but comes with limitations in its use.
This class is likewise provided for extracting arrays out of a :class:`viskores::cont::UnknownArrayHandle` (see :secref:`unknown-array-handle:Extracting An Unknown Amount of Components`).

.. doxygenclass:: viskores::cont::ArrayHandleRecombineVec
   :members:
