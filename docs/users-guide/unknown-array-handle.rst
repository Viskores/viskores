==============================
Unknown Array Handles
==============================

.. index::
   single: unknown array handle
   single: array handle; unknown

The :class:`vtkm::cont::ArrayHandle` class uses templating to make very efficient and type-safe access to data.
However, it is sometimes inconvenient or impossible to specify the element type and storage at run-time.
The :class:`vtkm::cont::UnknownArrayHandle` class provides a mechanism to manage arrays of data with unspecified types.

:class:`vtkm::cont::UnknownArrayHandle` holds a reference to an array.
Unlike :class:`vtkm::cont::ArrayHandle`, :class:`vtkm::cont::UnknownArrayHandle` is *not* templated.
Instead, it uses C++ run-type type information to store the array without type and cast it when appropriate.

.. doxygenclass:: vtkm::cont::UnknownArrayHandle

.. index:: unknown array handle; construct

An :class:`vtkm::cont::UnknownArrayHandle` can be established by constructing it with or assigning it to an :class:`vtkm::cont::ArrayHandle`.
The following example demonstrates how an :class:`vtkm::cont::UnknownArrayHandle` might be used to load an array whose type is not known until run-time.

.. load-example:: CreateUnknownArrayHandle
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Creating an :class:`vtkm::cont::UnknownArrayHandle`.

It is possible to construct a :class:`vtkm::cont::UnknownArrayHandle` that does not point to any :class:`vtkm::cont::ArrayHandle`.
In this case, the :class:`vtkm::cont::UnknownArrayHandle` is considered not "valid."
Validity can be tested with the :func:`vtkm::cont::UnknownArrayHandle::IsValid` method.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::IsValid

Most of the following operations on :class:`vtkm::cont::UnknownArrayHandle` will fail by throwing an exception if it is not valid.
Note that it is also possible for a :class:`vtkm::cont::UnknownArrayHandle` to contain an empty :class:`vtkm::cont::ArrayHandle`.
A :class:`vtkm::cont::UnknownArrayHandle` that contains a :class:`vtkm::cont::ArrayHandle` but has no memory allocated is still considered valid.

Some basic, human-readable information can be retrieved using the :func:`vtkm::cont::UnknownArrayHandle::PrintSummary` method.
It will print the type and size of the array along with some or all of the values.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::PrintSummary


------------------------------
Allocation
------------------------------

.. index:: unknown array handle; allocation

Data pointed to by an :class:`vtkm::cont::UnknownArrayHandle` is not directly accessible.
However, it is still possible to do some type-agnostic manipulation of the array allocations.

First, it is always possible to call :func:`vtkm::cont::UnknownArrayHandle::GetNumberOfValues` to retrieve the current size of the array.
It is also possible to call :func:`vtkm::cont::UnknownArrayHandle::Allocate` to change the size of an unknown array.
:class:`vtkm::cont::UnknownArrayHandle`'s :func:`vtkm::cont::UnknownArrayHandle::Allocate` works exactly the same as the :func:`vtkm::cont::ArrayHandle::Allocate` in the basic :class:`vtkm::cont::ArrayHandle`.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::GetNumberOfValues
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::Allocate(vtkm::Id, vtkm::CopyFlag, vtkm::cont::Token&) const
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::Allocate(vtkm::Id, vtkm::CopyFlag) const

.. load-example:: UnknownArrayHandleResize
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Checking the size of a :class:`vtkm::cont::ArrayHandle` and resizing it.

It is often the case where you have an :class:`vtkm::cont::UnknownArrayHandle` as the input to an operation and you want to generate an output of the same type.
To handle this case, use the :func:`vtkm::cont::UnknownArrayHandle::NewInstance` method to create a new array of the same type (without having to determine the type).

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::NewInstance

.. load-example:: NonTypeUnknownArrayHandleNewInstance
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Creating a new instance of an unknown array handle.

That said, there are many special array handles described in :chapref:`memory-layout:Memory Layout of Array Handles` and :chapref:`fancy-array-handles:Fancy Array Handles` that either cannot be directly constructed or cannot be used as outputs.
Thus, if you do not know the storage of the array, the similar array returned by :func:`vtkm::cont::UnknownArrayHandle::NewInstance` could be infeasible for use as an output.
Thus, :class:`vtkm::cont::UnknownArrayHandle` also contains the :func:`vtkm::cont::UnknownArrayHandle::NewInstanceBasic` method to create a new array with the same value type but using the basic array storage, which can always be resized and written to.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::NewInstanceBasic

.. load-example:: UnknownArrayHandleBasicInstance
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Creating a new basic instance of an unknown array handle.

It is sometimes the case that you need a new array of a similar type, but that type has to hold floating point values.
For example, if you had an operation that computed a discrete cosine transform on an array, the result would be very inaccurate if stored as integers.
In this case, you would actually want to store the result in an array of floating point values.
For this case, you can use the :func:`vtkm::cont::UnknownArrayHandle::NewInstanceFloatBasic` to create a new basic :class:`vtkm::cont::ArrayHandle` with the component type changed to :type:`vtkm::FloatDefault`.
For example, if the :class:`vtkm::cont::UnknownArrayHandle` stores an :class:`vtkm::cont::ArrayHandle` of type :type:`vtkm::Id`, :func:`vtkm::cont::UnknownArrayHandle::NewInstanceFloatBasic` will create an :class:`vtkm::cont::ArrayHandle` of type :type:`vtkm::FloatDefault`.
If the :class:`vtkm::cont::UnknownArrayHandle` stores an :class:`vtkm::cont::ArrayHandle` of type :type:`vtkm::Id3`, :func:`vtkm::cont::UnknownArrayHandle::NewInstanceFloatBasic` will create an :class:`vtkm::cont::ArrayHandle` of type :type:`vtkm::Vec3f`.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::NewInstanceFloatBasic

.. load-example:: UnknownArrayHandleFloatInstance
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Creating a new array instance with floating point values.

Finally, it may be the case where you are finished using a :class:`vtkm::cont::UnknownArrayHandle`.
If you want to free up memory on the device, which may have limited memory, you can do so with :func:`vtkm::cont::UnknownArrayHandle::ReleaseResourcesExecution`, which will free any memory on the device but preserve the data on the host.
If the data will never be used again, all memory can be freed with :func:`vtkm::cont::UnknownArrayHandle::ReleaseResources`

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::ReleaseResourcesExecution
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::ReleaseResources


------------------------------
Casting to Known Types
------------------------------

.. index::
   single: unknown array handle; cast
   single: unknown array handle; as array handle

Data pointed to by an :class:`vtkm::cont::UnknownArrayHandle` is not directly
accessible.
To access the data, you need to retrieve the data as an :class:`vtkm::cont::ArrayHandle`.
If you happen to know (or can guess) the type, you can use the :func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle` method to retrieve the array as a specific type.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::AsArrayHandle(vtkm::cont::ArrayHandle<T, S>&) const
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::AsArrayHandle() const

.. load-example:: UnknownArrayHandleAsArrayHandle1
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Retrieving an array of a known type from :class:`vtkm::cont::UnknownArrayHandle`.

:func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle` actually has two forms.
The first form, shown in the previous example, has no arguments and returns the :class:`vtkm::cont::ArrayHandle`.
This form requires you to specify the type of array as a template parameter.
The alternate form has you pass a reference to a concrete :class:`vtkm::cont::ArrayHandle` as an argument as shown in the following example.
This form can imply the template parameter from the argument.

.. load-example:: UnknownArrayHandleAsArrayHandle2
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Alternate form for retrieving an array of a known type from :class:`vtkm::cont::UnknownArrayHandle`.

:func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle` treats :class:`vtkm::cont::ArrayHandleCast` and :class:`vtkm::cont::ArrayHandleMultiplexer` special.
If the special :class:`vtkm::cont::ArrayHandle` can hold the actual array stored, then :func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle` will return successfully.
In the following example, :func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle` returns an array of type :type:`vtkm::Float32` as an :class:`vtkm::cont::ArrayHandleCast` that converts the values to :type:`vtkm::Float64`.

.. load-example:: UnknownArrayHandleAsCastArray
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Getting a cast array handle from an :class:`vtkm::cont::ArrayHandleCast`.

.. didyouknow::
   The inverse retrieval works as well.
   If you create an :class:`vtkm::cont::UnknownArrayHandle` with an :class:`vtkm::cont::ArrayHandleCast` or :class:`vtkm::cont::ArrayHandleMultiplexer`, you can get the underlying array with :func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle`.
   These relationships also work recursively (e.g. an array placed in a cast array that is placed in a multiplexer).

.. index:: unknown array handle; query type

If the :class:`vtkm::cont::UnknownArrayHandle` cannot store its array in the type given to :func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle`, it will throw an exception.
Thus, you should not use :func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle` with types that you are not sure about.
Use the :func:`vtkm::cont::UnknownArrayHandle::CanConvert` method to determine if a given :class:`vtkm::cont::ArrayHandle` type will work with :func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle`.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::CanConvert

.. load-example:: UnknownArrayHandleCanConvert
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Querying whether a given :class:`vtkm::cont::ArrayHandle` can be retrieved from a :class:`vtkm::cont::UnknownArrayHandle`.

By design, :func:`vtkm::cont::UnknownArrayHandle::CanConvert` will return true for types that are not actually stored in the :class:`vtkm::cont::UnknownArrayHandle` but can be retrieved.
If you need to know specifically what type is stored in the :class:`vtkm::cont::UnknownArrayHandle`, you can use the :func:`vtkm::cont::UnknownArrayHandle::IsType` method instead.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::IsType

If you need to query either the value type or the storage, you can use :func:`vtkm::cont::UnknownArrayHandle::IsValueType` and :func:`vtkm::cont::UnknownArrayHandle::IsStorageType`, respectively.
:class:`vtkm::cont::UnknownArrayHandle` also provides :func:`vtkm::cont::UnknownArrayHandle::GetValueTypeName`, :func:`vtkm::cont::UnknownArrayHandle::GetStorageTypeName`, and :func:`vtkm::cont::UnknownArrayHandle::GetArrayTypeName` for debugging purposes.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::IsValueType
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::IsStorageType
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::GetValueTypeName
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::GetStorageTypeName
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::GetArrayTypeName

.. commonerrors::
   :func:`vtkm::cont::UnknownArrayHandle::CanConvert` is almost always safer to use than :func:`vtkm::cont::UnknownArrayHandle::IsType` or its similar methods.
   Even though :func:`vtkm::cont::UnknownArrayHandle::IsType` reflects the actual array type, :func:`vtkm::cont::UnknownArrayHandle::CanConvert` better describes how :class:`vtkm::cont::UnknownArrayHandle` will behave.

If you do not know the exact type of the array contained in an :class:`vtkm::cont::UnknownArrayHandle`, a brute force method to get the data out is to copy it to an array of a known type.
This can be done with the :func:`vtkm::cont::UnknownArrayHandle::DeepCopyFrom` method, which will copy the contents of a target array into an existing array of a (potentially) different type.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::DeepCopyFrom(const vtkm::cont::UnknownArrayHandle&)
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::DeepCopyFrom(const vtkm::cont::UnknownArrayHandle&) const

.. load-example:: UnknownArrayHandleDeepCopy
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Deep copy arrays of unknown types.

It is often the case that you have good reason to believe that an array is of an expected type, but you have no way to be sure.
To simplify code, the most rational thing to do is to get the array as the expected type if that is indeed what it is, or to copy it to an array of that type otherwise.
The :func:`vtkm::cont::UnknownArrayHandle::CopyShallowIfPossible` does just that.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::CopyShallowIfPossible(const vtkm::cont::UnknownArrayHandle&)
.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::CopyShallowIfPossible(const vtkm::cont::UnknownArrayHandle&) const

.. load-example:: UnknownArrayHandleShallowCopy
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Using :func:`vtkm::cont::UnknownArrayHandle::CopyShallowIfPossible` to get an unknown array as a particular type.

.. didyouknow::
   The :class:`vtkm::cont::UnknownArrayHandle` copy methods behave similarly to the :func:`vtkm::cont::ArrayCopy` functions.


----------------------------------------
Casting to a List of Potential Types
----------------------------------------

.. index:: unknown array handle; cast

Using :func:`vtkm::cont::UnknownArrayHandle::AsArrayHandle` is fine as long as the correct types are known, but often times they are not.
For this use case :class:`vtkm::cont::UnknownArrayHandle` has a method named :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypes` that attempts to cast the array to some set of types.

The :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypes` method accepts a functor to run on the appropriately cast array.
The functor must have an overloaded const parentheses operator that accepts an :class:`vtkm::cont::ArrayHandle` of the appropriate type.
You also have to specify two template parameters that specify a :class:`vtkm::List` of value types to try and a :class:`vtkm::List` of storage types to try, respectively.
The macros :c:macro:`VTKM_DEFAULT_TYPE_LIST` and :c:macro:`VTKM_DEFAULT_STORAGE_LIST` are often used when nothing more specific is known.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::CastAndCallForTypes

.. load-example:: UsingCastAndCallForTypes
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Operating on an :class:`vtkm::cont::UnknownArrayHandle` with :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypes`.

.. didyouknow::
   The first (required) argument to :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypes` is the functor to call with the array.
   You can supply any number of optional arguments after that.
   Those arguments will be passed directly to the functor.
   This makes it easy to pass state to the functor.

.. didyouknow::
   When an :class:`vtkm::cont::UnknownArrayHandle` is used in place of an :class:`vtkm::cont::ArrayHandle` as an argument to a worklet invocation, it will internally use :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypes` to attempt to call the worklet with an :class:`vtkm::cont::ArrayHandle` of the correct type.

:class:`vtkm::cont::UnknownArrayHandle` has a simple subclass named :class:`vtkm::cont::UncertainArrayHandle` for use when you can narrow the array to a finite set of types.
:class:`vtkm::cont::UncertainArrayHandle` has two template parameters that must be specified: a :class:`vtkm::List` of value types and a :class:`vtkm::List` of storage types.

.. doxygenclass:: vtkm::cont::UncertainArrayHandle

:class:`vtkm::cont::UncertainArrayHandle` has a method named :func:`vtkm::cont::UncertainArrayHandle::CastAndCall` that behaves the same as :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypes` except that you do not have to specify the types to try.
Instead, the types are taken from the template parameters of the :class:`vtkm::cont::UncertainArrayHandle` itself.

.. doxygenfunction:: vtkm::cont::UncertainArrayHandle::CastAndCall

.. load-example:: UncertainArrayHandle
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Using :class:`vtkm::cont::UncertainArrayHandle` to cast and call a functor.

.. didyouknow::
   Like with :class:`vtkm::cont::UnknownArrayHandle`, if an :class:`vtkm::cont::UncertainArrayHandle` is used in a worklet invocation, it will internally use :func:`vtkm::cont::UncertainArrayHandle::CastAndCall`.
   This provides a convenient way to specify what array types the invoker should try.

Both :class:`vtkm::cont::UnknownArrayHandle` and :class:`vtkm::cont::UncertainArrayHandle` provide a method named :func:`vtkm::cont::UnknownArrayHandle::ResetTypes` to redefine the types to try.
:func:`vtkm::cont::UncertainArrayHandle::ResetTypes` has two template parameters that are the :class:`vtkm::List`'s of value and storage types.
:func:`vtkm::cont::UnknownArrayHandle::ResetTypes` returns a new :class:`vtkm::cont::UncertainArrayHandle` with the given types.
This is a convenient way to pass these types to functions.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::ResetTypes

:class:`vtkm::cont::UncertainArrayHandle` additionally has methods named :func:`vtkm::cont::UncertainArrayHandle::ResetValueTypes` and :func:`vtkm::cont::UncertainArrayHandle::ResetStorageTypes` to reset the value types and storage types, respectively, without modifying the other.

.. doxygenfunction:: vtkm::cont::UncertainArrayHandle::ResetValueTypes
.. doxygenfunction:: vtkm::cont::UncertainArrayHandle::ResetStorageTypes

.. load-example:: UnknownArrayResetTypes
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Resetting the types of an :class:`vtkm::cont::UnknownArrayHandle`.

.. commonerrors::
   Because it returns an :class:`vtkm::cont::UncertainArrayHandle`, you need to include :file:`vtkm/cont/UncertainArrayHandle.h` if you use :func:`vtkm::cont::UnknownArrayHandle::ResetTypes`.
   This is true even if you do not directly use the returned object.


------------------------------
Accessing Truly Unknown Arrays
------------------------------

So far in :secref:`unknown-array-handle:Casting to Known Types` and :secref:`unknown-array-handle:Casting to a List of Potential Types` we explored how to access the data in an :class:`vtkm::cont::UnknownArrayHandle` when you actually know the array type or can narrow down the array type to some finite number of candidates.
But what happens if you cannot practically narrow down the types in the :class:`vtkm::cont::UnknownArrayHandle`?
For this case, :class:`vtkm::cont::UnknownArrayHandle` provides mechanisms for extracting data knowing little or nothing about the types.

Cast with Floating Point Fallback
========================================

.. index:: unknown array handle; fallback

The problem with :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypes` and :func:`vtkm::cont::UncertainArrayHandle::CastAndCall` is that you can only list a finite amount of value types and storage types to try.
If you encounter an :class:`vtkm::cont::UnknownArrayHandle` containing a different :class:`vtkm::cont::ArrayHandle` type, the cast and call will simply fail.
Since the compiler must create a code path for each possible :class:`vtkm::cont::ArrayHandle` type, it may not even be feasible to list all known types.

:func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback` works around this problem by providing a fallback in case the contained :class:`vtkm::cont::ArrayHandle` does not match any of the types tried.
If none of the types match, then :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback` will copy the data to a :class:`vtkm::cont::ArrayHandle` with :type:`vtkm::FloatDefault` values (or some compatible :class:`vtkm::Vec` with :type:`vtkm::FloatDefault` components) and basic storage.
It will then attempt to match again with this copied array.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback

.. load-example:: CastAndCallForTypesWithFloatFallback
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Cast and call a functor from an :class:`vtkm::cont::UnknownArrayHandle` with a float fallback.

In this case, we do not have to list every possible type because the array will be copied to a known type if nothing matches.
Note that when using :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback`, you still need to include an appropriate type based on :type:`vtkm::FloatDefault` in the value type list and :class:`vtkm::cont::StorageTagBasic` in the storage list so that the copied array can match.

:class:`vtkm::cont::UncertainArrayHandle` has a matching method named :func:`vtkm::cont::UncertainArrayHandle::CastAndCallWithFloatFallback` that does the same operation using the types specified in the :class:`vtkm::cont::UncertainArrayHandle`.

.. doxygenfunction:: vtkm::cont::UncertainArrayHandle::CastAndCallWithFloatFallback

.. load-example:: CastAndCallWithFloatFallback
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Cast and call a functor from an :class:`vtkm::cont::UncertainArrayHandle` with a float fallback.

Extracting Components
==============================

Using a floating point fallback allows you to use arrays of unknown types in most circumstances, but it does have a few drawbacks.
First, and most obvious, is that you may not operate on the data in its native format.
If you want to preserve the integer format of data, this may not be the method.
Second, the fallback requires a copy of the data.
If :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback` does not match the type of the array, it copies the array to a new type that (hopefully) can be matched.
Third, :func:`vtkm::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback` still needs to match the number of components in each array value.
If the contained :class:`vtkm::cont::ArrayHandle` contains values that are :class:`vtkm::Vec`'s of length 2, then the data will be copied to an array of :type:`vtkm::Vec2f`'s.
If :type:`vtkm::Vec2f` is not included in the types to try, the cast and call will still fail.

.. index:: unknown array handle; extract component

A way to get around these problems is to extract a single component from the array.
You can use the :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` method to return an :class:`vtkm::cont::ArrayHandle` with the values for a given component for each value in the array.
The type of the returned :class:`vtkm::cont::ArrayHandle` will be the same regardless of the actual array type stored in the :class:`vtkm::cont::UnknownArrayHandle`.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::ExtractComponent

:func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` must be given a template argument for the base component type.
The following example extracts the first component of all :class:`vtkm::Vec` values in an :class:`vtkm::cont::UnknownArrayHandle` assuming that the component is of type :type:`vtkm::FloatDefault` (:exlineref:`ex:UnknownArrayExtractComponent:Call`).

.. load-example:: UnknownArrayExtractComponent
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Extracting the first component of every value in an :class:`vtkm::cont::UnknownArrayHandle`.

The code in :numref:`ex:UnknownArrayExtractComponent` works with any array with values based on the default floating point type.
If the :class:`vtkm::cont::UnknownArrayHandle` has an array containing :type:`vtkm::FloatDefault`, then the returned array has all the same values.
If the :class:`vtkm::cont::UnknownArrayHandle` contains values of type :type:`vtkm::Vec3f`, then each value in the returned array will be the first component of this array.

If the :class:`vtkm::cont::UnknownArrayHandle` really contains an array with incompatible value types (such as ``vtkm::cont::ArrayHandle<vtkm::Id>``), then an :class:`vtkm::cont::ErrorBadType` will be thrown.
To check if the :class:`vtkm::cont::UnknownArrayHandle` contains an array of a compatible type, use the :func:`vtkm::cont::UnknownArrayHandle::IsBaseComponentType` method to check the component type being used as the template argument to :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent`.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::IsBaseComponentType

.. load-example:: UnknownArrayBaseComponentType
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Checking the base component type in an :class:`vtkm::cont::UnknownArrayHandle`.

it is also possible to get a name for the base component type (mostly for debugging purposes) with :func:`vtkm::cont::UnknownArrayHandle::GetBaseComponentTypeName`.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::GetBaseComponentTypeName

You will often need to query the number of components that can be extracted from the array.
This can be queried with :func:`vtkm::cont::UnknownArrayHandle::GetNumberOfComponentsFlat`.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::GetNumberOfComponentsFlat

This section started with the motivation of getting data from an :class:`vtkm::cont::UnknownArrayHandle` without knowing anything about the type, yet :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` still requires a type parameter.
However, by limiting the type needed to the base component type, you only need to check the base C types (standard integers and floating points) available in C++.
You do not need to know whether these components are arranged in :class:`vtkm::Vec`'s or the size of the :class:`vtkm::Vec`.
A general implementation of an algorithm might have to deal with scalars as well as :class:`vtkm::Vec`'s of size 2, 3, and 4.
If we consider operations on tensors, :class:`vtkm::Vec`'s of size 6 and 9 can be common as well.
But when using :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent`, a single condition can handle any potential :class:`vtkm::Vec` size.

Another advantage of :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` is that the type of storage does not need to be specified.
:func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` works with any type of :class:`vtkm::cont::ArrayHandle` storage (with some caveats).
So, :numref:`ex:UnknownArrayExtractComponent` works equally as well with :class:`vtkm::cont::ArrayHandleBasic`, :class:`vtkm::cont::ArrayHandleSOA`, :class:`vtkm::cont::ArrayHandleUniformPointCoordinates`, :class:`vtkm::cont::ArrayHandleCartesianProduct`, and many others.
Trying to capture all reasonable types of arrays could easily require hundreds of conditions, all of which and more can be captured with :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` and the roughly 12 basic C data types.
In practice, you often only really have to worry about floating point components, which further reduces the cases down to (usually) 2.

:func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` works by returning an :class:`vtkm::cont::ArrayHandleStride`.
This is a special :class:`vtkm::cont::ArrayHandle` that can access data buffers by skipping values at regular intervals.
This allows it to access data packed in different ways such as :class:`vtkm::cont::ArrayHandleBasic`, :class:`vtkm::cont::ArrayHandleSOA`, and many others.
That said, :class:`vtkm::cont::ArrayHandleStride` is not magic, so if it cannot directly access memory, some or all of it may be copied.
If you are attempting to use the array from :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` as an output array, pass :enumerator:`vtkm::CopyFlag::Off` as a second argument.
This will ensure that data are not copied so that any data written will go to the original array (or throw an exception if this cannot be done).

.. commonerrors::
   Although :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` will technically work with any :class:`vtkm::cont::ArrayHandle` (of simple :class:`vtkm::Vec` types), it may require a very inefficient memory copy.
   Pay attention if :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` issues a warning about an inefficient memory copy.
   This is likely a serious performance issue, and the data should be retrieved in a different way (or better yet stored in a different way).

Extracting All Components
==============================

:numref:`ex:UnknownArrayExtractComponent` accesses the first component of each :class:`vtkm::Vec` in an array.
But in practice you usually want to operate on all components stored in the array.
A simple solution is to iterate over each component.

.. load-example:: UnknownArrayExtractComponentsMultiple
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Extracting each component from an :class:`vtkm::cont::UnknownArrayHandle`.

To ensure that the type of the extracted component is a basic C type, the :class:`vtkm::Vec` values are "flattened."
That is, they are treated as if they are a single level :class:`vtkm::Vec`.
For example, if you have a value type of ``vtkm::Vec<vtkm::Id3, 2>``, :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` treats this type as ``vtkm::Vec<vtkm::Id, 6>``.
This allows you to extract the components as type :type:`vtkm::Id` rather than having a special case for :type:`vtkm::Id3`.

Although iterating over components works fine, it can be inconvenient.
An alternate mechanism is to use :func:`vtkm::cont::UnknownArrayHandle::ExtractArrayFromComponents` to get all the components at once.
:func:`vtkm::cont::UnknownArrayHandle::ExtractArrayFromComponents` works like :func:`vtkm::cont::UnknownArrayHandle::ExtractComponent` except that instead of returning an :class:`vtkm::cont::ArrayHandleStride`, it returns a special :class:`vtkm::cont::ArrayHandleRecombineVec` that behaves like an :class:`vtkm::cont::ArrayHandle` to reference all component arrays at once.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::ExtractArrayFromComponents

.. load-example:: UnknownArrayExtractArrayFromComponents
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Extracting all components from an :class:`vtkm::cont::UnknownArrayHandle` at once.

.. commonerrors::
   Although it has the same interface as other :class:`vtkm::cont::ArrayHandle`'s, :class:`vtkm::cont::ArrayHandleRecombineVec` has a special value type that breaks some conventions.
   For example, when used in a worklet, the value type passed from this array to the worklet cannot be replicated.
   That is, you cannot create a temporary stack value of the same type.

Because you still need to specify a base component type, you will likely still need to check several types to safely extract data from an :class:`vtkm::cont::UnknownArrayHandle` by component.
To do this automatically, you can use the :func:`vtkm::cont::UnknownArrayHandle::CastAndCallWithExtractedArray`.
This method behaves similarly to :func:`vtkm::cont::UncertainArrayHandle::CastAndCall` except that it internally uses :func:`vtkm::cont::UnknownArrayHandle::ExtractArrayFromComponents`.

.. doxygenfunction:: vtkm::cont::UnknownArrayHandle::CastAndCallWithExtractedArray

.. load-example:: UnknownArrayCallWithExtractedArray
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Calling a functor for nearly any type of array stored in an :class:`vtkm::cont::UnknownArrayHandle`.


------------------------------
Mutability
------------------------------

.. index:: unknown array handle; const

One subtle feature of :class:`vtkm::cont::UnknownArrayHandle` is that the class is, in principle, a pointer to an array pointer.
This means that the data in an :class:`vtkm::cont::UnknownArrayHandle` is always mutable even if the class is declared ``const``.
The upshot is that you can pass output arrays as constant :class:`vtkm::cont::UnknownArrayHandle` references.

.. load-example:: UnknownArrayConstOutput
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Using a ``const`` :class:`vtkm::cont::UnknownArrayHandle` for a function output.

Although it seems strange, there is a good reason to allow an output :class:`vtkm::cont::UnknownArrayHandle` to be ``const``.
It allows a typed :class:`vtkm::cont::ArrayHandle` to be used as the argument to the function.
In this case, the compiler will automatically convert the :class:`vtkm::cont::ArrayHandle` to a :class:`vtkm::cont::UnknownArrayHandle`.
When C++ creates objects like this, they can only be passed a constant reference, an Rvalue reference, or by value.
So, declaring the output parameter as ``const`` :class:`vtkm::cont::UnknownArrayHandle` allows it to be used for code like this.

.. load-example:: UseUnknownArrayConstOutput
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Passing an :class:`vtkm::cont::ArrayHandle` as an output :class:`vtkm::cont::UnknownArrayHandle`.

Of course, you could also declare the output by value instead of by reference, but this has the same semantics with extra internal pointer management.

.. didyouknow::
   When possible, it is better to pass a :class:`vtkm::cont::UnknownArrayHandle` as a constant reference (or by value) rather than a mutable reference, even if the array contents are going to be modified.
   This allows the function to support automatic conversion of an output :class:`vtkm::cont::ArrayHandle`.

So if a constant :class:`vtkm::cont::UnknownArrayHandle` can have its contents modified, what is the difference between a constant reference and a non-constant reference?
The difference is that the constant reference can change the array's content, but not the array itself.
If you want to do operations like doing a shallow copy or changing the underlying type of the array, a non-constant reference is needed.
