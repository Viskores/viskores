==============================
Unknown Array Handles
==============================

.. index::
   single: unknown array handle
   single: array handle; unknown

The :class:`viskores::cont::ArrayHandle` class uses templating to make very efficient and type-safe access to data.
However, it is sometimes inconvenient or impossible to specify the element type and storage at run-time.
The :class:`viskores::cont::UnknownArrayHandle` class provides a mechanism to manage arrays of data with unspecified types.

:class:`viskores::cont::UnknownArrayHandle` holds a reference to an array.
Unlike :class:`viskores::cont::ArrayHandle`, :class:`viskores::cont::UnknownArrayHandle` is *not* templated.
Instead, it uses C++ run-type type information to store the array without type and cast it when appropriate.

.. doxygenclass:: viskores::cont::UnknownArrayHandle

.. index:: unknown array handle; construct

An :class:`viskores::cont::UnknownArrayHandle` can be established by constructing it with or assigning it to an :class:`viskores::cont::ArrayHandle`.
The following example demonstrates how an :class:`viskores::cont::UnknownArrayHandle` might be used to load an array whose type is not known until run-time.

.. load-example:: CreateUnknownArrayHandle
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Creating an :class:`viskores::cont::UnknownArrayHandle`.

It is possible to construct a :class:`viskores::cont::UnknownArrayHandle` that does not point to any :class:`viskores::cont::ArrayHandle`.
In this case, the :class:`viskores::cont::UnknownArrayHandle` is considered not "valid."
Validity can be tested with the :func:`viskores::cont::UnknownArrayHandle::IsValid` method.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::IsValid

Most of the following operations on :class:`viskores::cont::UnknownArrayHandle` will fail by throwing an exception if it is not valid.
Note that it is also possible for a :class:`viskores::cont::UnknownArrayHandle` to contain an empty :class:`viskores::cont::ArrayHandle`.
A :class:`viskores::cont::UnknownArrayHandle` that contains a :class:`viskores::cont::ArrayHandle` but has no memory allocated is still considered valid.

Some basic, human-readable information can be retrieved using the :func:`viskores::cont::UnknownArrayHandle::PrintSummary` method.
It will print the type and size of the array along with some or all of the values.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::PrintSummary


------------------------------
Allocation
------------------------------

.. index:: unknown array handle; allocation

Data pointed to by an :class:`viskores::cont::UnknownArrayHandle` is not directly accessible.
However, it is still possible to do some type-agnostic manipulation of the array allocations.

First, it is always possible to call :func:`viskores::cont::UnknownArrayHandle::GetNumberOfValues` to retrieve the current size of the array.
It is also possible to call :func:`viskores::cont::UnknownArrayHandle::Allocate` to change the size of an unknown array.
:class:`viskores::cont::UnknownArrayHandle`'s :func:`viskores::cont::UnknownArrayHandle::Allocate` works exactly the same as the :func:`viskores::cont::ArrayHandle::Allocate` in the basic :class:`viskores::cont::ArrayHandle`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::GetNumberOfValues
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::Allocate(viskores::Id, viskores::CopyFlag, viskores::cont::Token&) const
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::Allocate(viskores::Id, viskores::CopyFlag) const

.. load-example:: UnknownArrayHandleResize
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Checking the size of a :class:`viskores::cont::ArrayHandle` and resizing it.

It is often the case where you have an :class:`viskores::cont::UnknownArrayHandle` as the input to an operation and you want to generate an output of the same type.
To handle this case, use the :func:`viskores::cont::UnknownArrayHandle::NewInstance` method to create a new array of the same type (without having to determine the type).

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::NewInstance

.. load-example:: NonTypeUnknownArrayHandleNewInstance
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Creating a new instance of an unknown array handle.

That said, there are many special array handles described in :chapref:`memory-layout:Memory Layout of Array Handles` and :chapref:`fancy-array-handles:Fancy Array Handles` that either cannot be directly constructed or cannot be used as outputs.
Thus, if you do not know the storage of the array, the similar array returned by :func:`viskores::cont::UnknownArrayHandle::NewInstance` could be infeasible for use as an output.
Thus, :class:`viskores::cont::UnknownArrayHandle` also contains the :func:`viskores::cont::UnknownArrayHandle::NewInstanceBasic` method to create a new array with the same value type but using the basic array storage, which can always be resized and written to.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::NewInstanceBasic

.. load-example:: UnknownArrayHandleBasicInstance
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Creating a new basic instance of an unknown array handle.

It is sometimes the case that you need a new array of a similar type, but that type has to hold floating point values.
For example, if you had an operation that computed a discrete cosine transform on an array, the result would be very inaccurate if stored as integers.
In this case, you would actually want to store the result in an array of floating point values.
For this case, you can use the :func:`viskores::cont::UnknownArrayHandle::NewInstanceFloatBasic` to create a new basic :class:`viskores::cont::ArrayHandle` with the component type changed to :type:`viskores::FloatDefault`.
For example, if the :class:`viskores::cont::UnknownArrayHandle` stores an :class:`viskores::cont::ArrayHandle` of type :type:`viskores::Id`, :func:`viskores::cont::UnknownArrayHandle::NewInstanceFloatBasic` will create an :class:`viskores::cont::ArrayHandle` of type :type:`viskores::FloatDefault`.
If the :class:`viskores::cont::UnknownArrayHandle` stores an :class:`viskores::cont::ArrayHandle` of type :type:`viskores::Id3`, :func:`viskores::cont::UnknownArrayHandle::NewInstanceFloatBasic` will create an :class:`viskores::cont::ArrayHandle` of type :type:`viskores::Vec3f`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::NewInstanceFloatBasic

.. load-example:: UnknownArrayHandleFloatInstance
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Creating a new array instance with floating point values.

Finally, it may be the case where you are finished using a :class:`viskores::cont::UnknownArrayHandle`.
If you want to free up memory on the device, which may have limited memory, you can do so with :func:`viskores::cont::UnknownArrayHandle::ReleaseResourcesExecution`, which will free any memory on the device but preserve the data on the host.
If the data will never be used again, all memory can be freed with :func:`viskores::cont::UnknownArrayHandle::ReleaseResources`

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::ReleaseResourcesExecution
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::ReleaseResources


------------------------------
Casting to Known Types
------------------------------

.. index::
   single: unknown array handle; cast
   single: unknown array handle; as array handle

Data pointed to by an :class:`viskores::cont::UnknownArrayHandle` is not directly
accessible.
To access the data, you need to retrieve the data as an :class:`viskores::cont::ArrayHandle`.
If you happen to know (or can guess) the type, you can use the :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` method to retrieve the array as a specific type.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::AsArrayHandle(viskores::cont::ArrayHandle<T, S>&) const
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::AsArrayHandle() const

.. load-example:: UnknownArrayHandleAsArrayHandle1
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Retrieving an array of a known type from :class:`viskores::cont::UnknownArrayHandle`.

:func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` actually has two forms.
The first form, shown in the previous example, has no arguments and returns the :class:`viskores::cont::ArrayHandle`.
This form requires you to specify the type of array as a template parameter.
The alternate form has you pass a reference to a concrete :class:`viskores::cont::ArrayHandle` as an argument as shown in the following example.
This form can imply the template parameter from the argument.

.. load-example:: UnknownArrayHandleAsArrayHandle2
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Alternate form for retrieving an array of a known type from :class:`viskores::cont::UnknownArrayHandle`.

:func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` treats :class:`viskores::cont::ArrayHandleCast` and :class:`viskores::cont::ArrayHandleMultiplexer` special.
If the special :class:`viskores::cont::ArrayHandle` can hold the actual array stored, then :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` will return successfully.
In the following example, :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` returns an array of type :type:`viskores::Float32` as an :class:`viskores::cont::ArrayHandleCast` that converts the values to :type:`viskores::Float64`.

.. load-example:: UnknownArrayHandleAsCastArray
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Getting a cast array handle from an :class:`viskores::cont::ArrayHandleCast`.

.. didyouknow::
   The inverse retrieval works as well.
   If you create an :class:`viskores::cont::UnknownArrayHandle` with an :class:`viskores::cont::ArrayHandleCast` or :class:`viskores::cont::ArrayHandleMultiplexer`, you can get the underlying array with :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle`.
   These relationships also work recursively (e.g. an array placed in a cast array that is placed in a multiplexer).

.. index:: unknown array handle; query type

If the :class:`viskores::cont::UnknownArrayHandle` cannot store its array in the type given to :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle`, it will throw an exception.
Thus, you should not use :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` with types that you are not sure about.
Use the :func:`viskores::cont::UnknownArrayHandle::CanConvert` method to determine if a given :class:`viskores::cont::ArrayHandle` type will work with :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::CanConvert

.. load-example:: UnknownArrayHandleCanConvert
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Querying whether a given :class:`viskores::cont::ArrayHandle` can be retrieved from a :class:`viskores::cont::UnknownArrayHandle`.

By design, :func:`viskores::cont::UnknownArrayHandle::CanConvert` will return true for types that are not actually stored in the :class:`viskores::cont::UnknownArrayHandle` but can be retrieved.
If you need to know specifically what type is stored in the :class:`viskores::cont::UnknownArrayHandle`, you can use the :func:`viskores::cont::UnknownArrayHandle::IsType` method instead.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::IsType

If you need to query either the value type or the storage, you can use :func:`viskores::cont::UnknownArrayHandle::IsValueType` and :func:`viskores::cont::UnknownArrayHandle::IsStorageType`, respectively.
:class:`viskores::cont::UnknownArrayHandle` also provides :func:`viskores::cont::UnknownArrayHandle::GetValueTypeName`, :func:`viskores::cont::UnknownArrayHandle::GetStorageTypeName`, and :func:`viskores::cont::UnknownArrayHandle::GetArrayTypeName` for debugging purposes.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::IsValueType
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::IsStorageType
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::GetValueTypeName
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::GetStorageTypeName
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::GetArrayTypeName

.. commonerrors::
   :func:`viskores::cont::UnknownArrayHandle::CanConvert` is almost always safer to use than :func:`viskores::cont::UnknownArrayHandle::IsType` or its similar methods.
   Even though :func:`viskores::cont::UnknownArrayHandle::IsType` reflects the actual array type, :func:`viskores::cont::UnknownArrayHandle::CanConvert` better describes how :class:`viskores::cont::UnknownArrayHandle` will behave.

If you do not know the exact type of the array contained in an :class:`viskores::cont::UnknownArrayHandle`, a brute force method to get the data out is to copy it to an array of a known type.
This can be done with the :func:`viskores::cont::UnknownArrayHandle::DeepCopyFrom` method, which will copy the contents of a target array into an existing array of a (potentially) different type.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::DeepCopyFrom(const viskores::cont::UnknownArrayHandle&)
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::DeepCopyFrom(const viskores::cont::UnknownArrayHandle&) const

.. load-example:: UnknownArrayHandleDeepCopy
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Deep copy arrays of unknown types.

.. didyouknow::
   The :class:`viskores::cont::UnknownArrayHandle` copy methods behave similarly to the :func:`viskores::cont::ArrayCopy` functions.

It is often the case that you have good reason to believe that an array is of an expected type, but you have no way to be sure.
To simplify code, the most rational thing to do is to get the array as the expected type if that is indeed what it is, or to copy it to an array of that type otherwise.
The :func:`viskores::cont::ArrayCopyShallowIfPossible` does just that.

.. doxygenfunction:: viskores::cont::ArrayCopyShallowIfPossible

.. load-example:: ArrayCopyShallow
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Using :func:`viskores::cont::ArrayCopyShallowIfPossible` to get an unknown array as a particular type.

:class:`viskores::cont::UnknownArrayHandle` also has a method to do a similar shallow copy into it.
This method works by setting an array of a particular type into the :class:`viskores::cont::UnknownArrayHandle`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::CopyShallowIfPossible(const viskores::cont::UnknownArrayHandle&)
.. doxygenfunction:: viskores::cont::UnknownArrayHandle::CopyShallowIfPossible(const viskores::cont::UnknownArrayHandle&) const

.. load-example:: UnknownArrayHandleShallowCopy
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Using :func:`viskores::cont::UnknownArrayHandle::CopyShallowIfPossible` to get an unknown array as a particular type.


----------------------------------------
Casting to a List of Potential Types
----------------------------------------

.. index:: unknown array handle; cast

Using :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` is fine as long as the correct types are known, but often times they are not.
For this use case :class:`viskores::cont::UnknownArrayHandle` has a method named :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypes` that attempts to cast the array to some set of types.

The :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypes` method accepts a functor to run on the appropriately cast array.
The functor must have an overloaded const parentheses operator that accepts an :class:`viskores::cont::ArrayHandle` of the appropriate type.
You also have to specify two template parameters that specify a :class:`viskores::List` of value types to try and a :class:`viskores::List` of storage types to try, respectively.
The macros :c:macro:`VISKORES_DEFAULT_TYPE_LIST` and :c:macro:`VISKORES_DEFAULT_STORAGE_LIST` are often used when nothing more specific is known.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::CastAndCallForTypes

.. load-example:: UsingCastAndCallForTypes
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Operating on an :class:`viskores::cont::UnknownArrayHandle` with :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypes`.

.. didyouknow::
   The first (required) argument to :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypes` is the functor to call with the array.
   You can supply any number of optional arguments after that.
   Those arguments will be passed directly to the functor.
   This makes it easy to pass state to the functor.

.. didyouknow::
   When an :class:`viskores::cont::UnknownArrayHandle` is used in place of an :class:`viskores::cont::ArrayHandle` as an argument to a worklet invocation, it will internally use :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypes` to attempt to call the worklet with an :class:`viskores::cont::ArrayHandle` of the correct type.

:class:`viskores::cont::UnknownArrayHandle` has a simple subclass named :class:`viskores::cont::UncertainArrayHandle` for use when you can narrow the array to a finite set of types.
:class:`viskores::cont::UncertainArrayHandle` has two template parameters that must be specified: a :class:`viskores::List` of value types and a :class:`viskores::List` of storage types.

.. doxygenclass:: viskores::cont::UncertainArrayHandle

:class:`viskores::cont::UncertainArrayHandle` has a method named :func:`viskores::cont::UncertainArrayHandle::CastAndCall` that behaves the same as :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypes` except that you do not have to specify the types to try.
Instead, the types are taken from the template parameters of the :class:`viskores::cont::UncertainArrayHandle` itself.

.. doxygenfunction:: viskores::cont::UncertainArrayHandle::CastAndCall

.. load-example:: UncertainArrayHandle
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Using :class:`viskores::cont::UncertainArrayHandle` to cast and call a functor.

.. didyouknow::
   Like with :class:`viskores::cont::UnknownArrayHandle`, if an :class:`viskores::cont::UncertainArrayHandle` is used in a worklet invocation, it will internally use :func:`viskores::cont::UncertainArrayHandle::CastAndCall`.
   This provides a convenient way to specify what array types the invoker should try.

Both :class:`viskores::cont::UnknownArrayHandle` and :class:`viskores::cont::UncertainArrayHandle` provide a method named :func:`viskores::cont::UnknownArrayHandle::ResetTypes` to redefine the types to try.
:func:`viskores::cont::UncertainArrayHandle::ResetTypes` has two template parameters that are the :class:`viskores::List`'s of value and storage types.
:func:`viskores::cont::UnknownArrayHandle::ResetTypes` returns a new :class:`viskores::cont::UncertainArrayHandle` with the given types.
This is a convenient way to pass these types to functions.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::ResetTypes

:class:`viskores::cont::UncertainArrayHandle` additionally has methods named :func:`viskores::cont::UncertainArrayHandle::ResetValueTypes` and :func:`viskores::cont::UncertainArrayHandle::ResetStorageTypes` to reset the value types and storage types, respectively, without modifying the other.

.. doxygenfunction:: viskores::cont::UncertainArrayHandle::ResetValueTypes
.. doxygenfunction:: viskores::cont::UncertainArrayHandle::ResetStorageTypes

.. load-example:: UnknownArrayResetTypes
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Resetting the types of an :class:`viskores::cont::UnknownArrayHandle`.

.. commonerrors::
   Because it returns an :class:`viskores::cont::UncertainArrayHandle`, you need to include :file:`viskores/cont/UncertainArrayHandle.h` if you use :func:`viskores::cont::UnknownArrayHandle::ResetTypes`.
   This is true even if you do not directly use the returned object.


------------------------------
Accessing Truly Unknown Arrays
------------------------------

So far in :secref:`unknown-array-handle:Casting to Known Types` and :secref:`unknown-array-handle:Casting to a List of Potential Types` we explored how to access the data in an :class:`viskores::cont::UnknownArrayHandle` when you actually know the array type or can narrow down the array type to some finite number of candidates.
But what happens if you cannot practically narrow down the types in the :class:`viskores::cont::UnknownArrayHandle`?
For this case, :class:`viskores::cont::UnknownArrayHandle` provides mechanisms for extracting data knowing little or nothing about the types.

Cast with Floating Point Fallback
========================================

.. index:: unknown array handle; fallback

The problem with :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypes` and :func:`viskores::cont::UncertainArrayHandle::CastAndCall` is that you can only list a finite amount of value types and storage types to try.
If you encounter an :class:`viskores::cont::UnknownArrayHandle` containing a different :class:`viskores::cont::ArrayHandle` type, the cast and call will simply fail.
Since the compiler must create a code path for each possible :class:`viskores::cont::ArrayHandle` type, it may not even be feasible to list all known types.

:func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback` works around this problem by providing a fallback in case the contained :class:`viskores::cont::ArrayHandle` does not match any of the types tried.
If none of the types match, then :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback` will copy the data to a :class:`viskores::cont::ArrayHandle` with :type:`viskores::FloatDefault` values (or some compatible :class:`viskores::Vec` with :type:`viskores::FloatDefault` components) and basic storage.
It will then attempt to match again with this copied array.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback

.. load-example:: CastAndCallForTypesWithFloatFallback
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Cast and call a functor from an :class:`viskores::cont::UnknownArrayHandle` with a float fallback.

In this case, we do not have to list every possible type because the array will be copied to a known type if nothing matches.
Note that when using :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback`, you still need to include an appropriate type based on :type:`viskores::FloatDefault` in the value type list and :class:`viskores::cont::StorageTagBasic` in the storage list so that the copied array can match.

:class:`viskores::cont::UncertainArrayHandle` has a matching method named :func:`viskores::cont::UncertainArrayHandle::CastAndCallWithFloatFallback` that does the same operation using the types specified in the :class:`viskores::cont::UncertainArrayHandle`.

.. doxygenfunction:: viskores::cont::UncertainArrayHandle::CastAndCallWithFloatFallback

.. load-example:: CastAndCallWithFloatFallback
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Cast and call a functor from an :class:`viskores::cont::UncertainArrayHandle` with a float fallback.

Extracting Components
==============================

Using a floating point fallback allows you to use arrays of unknown types in most circumstances, but it does have a few drawbacks.
First, and most obvious, is that you may not operate on the data in its native format.
If you want to preserve the integer format of data, this may not be the method.
Second, the fallback requires a copy of the data.
If :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback` does not match the type of the array, it copies the array to a new type that (hopefully) can be matched.
Third, :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback` still needs to match the number of components in each array value.
If the contained :class:`viskores::cont::ArrayHandle` contains values that are :class:`viskores::Vec`'s of length 2, then the data will be copied to an array of :type:`viskores::Vec2f`'s.
If :type:`viskores::Vec2f` is not included in the types to try, the cast and call will still fail.

.. index:: unknown array handle; extract component

A way to get around these problems is to extract a single component from the array.
You can use the :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` method to return an :class:`viskores::cont::ArrayHandle` with the values for a given component for each value in the array.
The type of the returned :class:`viskores::cont::ArrayHandle` will be the same regardless of the actual array type stored in the :class:`viskores::cont::UnknownArrayHandle`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::ExtractComponent

:func:`viskores::cont::UnknownArrayHandle::ExtractComponent` must be given a template argument for the base component type.
The following example extracts the first component of all :class:`viskores::Vec` values in an :class:`viskores::cont::UnknownArrayHandle` assuming that the component is of type :type:`viskores::FloatDefault` (:exlineref:`ex:UnknownArrayExtractComponent:Call`).

.. load-example:: UnknownArrayExtractComponent
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Extracting the first component of every value in an :class:`viskores::cont::UnknownArrayHandle`.

The code in :numref:`ex:UnknownArrayExtractComponent` works with any array with values based on the default floating point type.
If the :class:`viskores::cont::UnknownArrayHandle` has an array containing :type:`viskores::FloatDefault`, then the returned array has all the same values.
If the :class:`viskores::cont::UnknownArrayHandle` contains values of type :type:`viskores::Vec3f`, then each value in the returned array will be the first component of this array.

If the :class:`viskores::cont::UnknownArrayHandle` really contains an array with incompatible value types (such as ``viskores::cont::ArrayHandle<viskores::Id>``), then an :class:`viskores::cont::ErrorBadType` will be thrown.
To check if the :class:`viskores::cont::UnknownArrayHandle` contains an array of a compatible type, use the :func:`viskores::cont::UnknownArrayHandle::IsBaseComponentType` method to check the component type being used as the template argument to :func:`viskores::cont::UnknownArrayHandle::ExtractComponent`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::IsBaseComponentType

.. load-example:: UnknownArrayBaseComponentType
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Checking the base component type in an :class:`viskores::cont::UnknownArrayHandle`.

it is also possible to get a name for the base component type (mostly for debugging purposes) with :func:`viskores::cont::UnknownArrayHandle::GetBaseComponentTypeName`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::GetBaseComponentTypeName

You will often need to query the number of components that can be extracted from the array.
This can be queried with :func:`viskores::cont::UnknownArrayHandle::GetNumberOfComponentsFlat`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::GetNumberOfComponentsFlat

This section started with the motivation of getting data from an :class:`viskores::cont::UnknownArrayHandle` without knowing anything about the type, yet :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` still requires a type parameter.
However, by limiting the type needed to the base component type, you only need to check the base C types (standard integers and floating points) available in C++.
You do not need to know whether these components are arranged in :class:`viskores::Vec`'s or the size of the :class:`viskores::Vec`.
A general implementation of an algorithm might have to deal with scalars as well as :class:`viskores::Vec`'s of size 2, 3, and 4.
If we consider operations on tensors, :class:`viskores::Vec`'s of size 6 and 9 can be common as well.
But when using :func:`viskores::cont::UnknownArrayHandle::ExtractComponent`, a single condition can handle any potential :class:`viskores::Vec` size.

Another advantage of :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` is that the type of storage does not need to be specified.
:func:`viskores::cont::UnknownArrayHandle::ExtractComponent` works with any type of :class:`viskores::cont::ArrayHandle` storage (with some caveats).
So, :numref:`ex:UnknownArrayExtractComponent` works equally as well with :class:`viskores::cont::ArrayHandleBasic`, :class:`viskores::cont::ArrayHandleSOA`, :class:`viskores::cont::ArrayHandleUniformPointCoordinates`, :class:`viskores::cont::ArrayHandleCartesianProduct`, and many others.
Trying to capture all reasonable types of arrays could easily require hundreds of conditions, all of which and more can be captured with :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` and the roughly 12 basic C data types.
In practice, you often only really have to worry about floating point components, which further reduces the cases down to (usually) 2.

:func:`viskores::cont::UnknownArrayHandle::ExtractComponent` works by returning an :class:`viskores::cont::ArrayHandleStride`.
This is a special :class:`viskores::cont::ArrayHandle` that can access data buffers by skipping values at regular intervals.
This allows it to access data packed in different ways such as :class:`viskores::cont::ArrayHandleBasic`, :class:`viskores::cont::ArrayHandleSOA`, and many others.
That said, :class:`viskores::cont::ArrayHandleStride` is not magic, so if it cannot directly access memory, some or all of it may be copied.
If you are attempting to use the array from :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` as an output array, pass :enumerator:`viskores::CopyFlag::Off` as a second argument.
This will ensure that data are not copied so that any data written will go to the original array (or throw an exception if this cannot be done).

.. commonerrors::
   Although :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` will technically work with any :class:`viskores::cont::ArrayHandle` (of simple :class:`viskores::Vec` types), it may require a very inefficient memory copy.
   Pay attention if :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` issues a warning about an inefficient memory copy.
   This is likely a serious performance issue, and the data should be retrieved in a different way (or better yet stored in a different way).

Extracting a Known Value Type from Unknown Storage
====================================================

:numref:`ex:UnknownArrayExtractComponent` accesses the first component of each :class:`viskores::Vec` in an array.
But in practice you usually want to operate on all components stored in the array.

If you can narrow down the type of each value in the array (both components and number of components), then you can extract all of the components at once with :func:`viskores::cont::UnknownArrayHandle::ExtractArrayWithValueType`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::ExtractArrayWithValueType

.. load-example:: UnknownArrayExtractArrayWithValueType
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Extracting an array with a known value type but unknown storage.

:func:`viskores::cont::UnknownArrayHandle::ExtractArrayWithValueType` works by extracting each component and building a :class:`viskores::cont::ArrayHandleSOAStride` array from them.
This array can be used as a drop-in replacement for other array handle types in templated parameters such as the invocation of a worklet.
An alternate mechanism is to use :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` to get an array of type :class:`viskores::cont::ArrayHandleSOAStride`.

.. load-example:: UnknownArrayAsSOAStride
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Extracting an array as a :class:`viskores::cont::ArrayHandleSOAStride`

.. didyouknow::
   :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` has special code to extract an array in the same way as :func:`viskores::cont::UnknownArrayHandle::ExtractArrayWithValueType`.
   The difference between the two is in which storage types will be accepted.
   :func:`viskores::cont::UnknownArrayHandle::ExtractArrayWithValueType` will technically accept any possible storage type, although some storage types can only be copied with an inefficient serial copy.
   (A warning will be issued in this case.)
   :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` will not convert any of these storage types with an inefficient copy.
   In contrast, :func:`viskores::cont::UnknownArrayHandle::ExtractArrayWithValueType` does provide an ``allowCopy`` argument that specifies whether the array can only be extracted with a shallow copy (for writing).
   There are some types that :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` will allow to have parts be shallow copied.

The important upshot of :func:`viskores::cont::UnknownArrayHandle::AsArrayHandle` having a special condition for :class:`viskores::cont::ArrayHandleSOAStride` to extract arrays is that a cast-and-call operation on the :class:`viskores::cont::UnknownArrayHandle` can have a fallback storage.

.. load-example:: UnknownArrayCastAndCallExtract
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Extracting an array from a selection of types and any storage.

This approach will also work on other cast-and-call techniques.
For example, it can be combined with :func:`viskores::cont::UnknownArrayHandle::CastAndCallForTypesWithFloatFallback` to cover cases where the correct type is not tried.

Extracting An Unknown Amount of Components
============================================

:numref:`ex:UnknownArrayExtractComponent` and :numref:`ex:UnknownArrayAsSOAStride` operate on an array of known value types and :numref:`ex:UnknownArrayCastAndCallExtract` operates on an array that is one of a limited number of value types.
However, there may be cases where the number of components in a value type cannot be known at compile time.
For example, an operation may operate on vectors of any size.
A simple solution may be to iterate over each component.

.. load-example:: UnknownArrayExtractComponentsMultiple
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Extracting each component from an :class:`viskores::cont::UnknownArrayHandle`.

To ensure that the type of the extracted component is a basic C type, the :class:`viskores::Vec` values are "flattened."
That is, they are treated as if they are a single level :class:`viskores::Vec`.
For example, if you have a value type of ``viskores::Vec<viskores::Id3, 2>``, :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` treats this type as ``viskores::Vec<viskores::Id, 6>``.
This allows you to extract the components as type :type:`viskores::Id` rather than having a special case for :type:`viskores::Id3`.

Although iterating over components works fine, it can be inconvenient.
An alternate mechanism is to use :func:`viskores::cont::UnknownArrayHandle::ExtractArrayFromComponents` to get all the components at once.
:func:`viskores::cont::UnknownArrayHandle::ExtractArrayFromComponents` works like :func:`viskores::cont::UnknownArrayHandle::ExtractComponent` except that instead of returning an :class:`viskores::cont::ArrayHandleStride`, it returns a special :class:`viskores::cont::ArrayHandleRecombineVec` that behaves like an :class:`viskores::cont::ArrayHandle` to reference all component arrays at once.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::ExtractArrayFromComponents

.. load-example:: UnknownArrayExtractArrayFromComponents
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Extracting all components from an :class:`viskores::cont::UnknownArrayHandle` at once.

.. commonerrors::
   Although it has the same interface as other :class:`viskores::cont::ArrayHandle`'s, :class:`viskores::cont::ArrayHandleRecombineVec` has a special value type that breaks some conventions.
   For example, when used in a worklet, the value type passed from this array to the worklet cannot be replicated.
   That is, you cannot create a temporary stack value of the same type.

Because you still need to specify a base component type, you will likely still need to check several types to safely extract data from an :class:`viskores::cont::UnknownArrayHandle` by component.
To do this automatically, you can use the :func:`viskores::cont::UnknownArrayHandle::CastAndCallWithExtractedArray`.
This method behaves similarly to :func:`viskores::cont::UncertainArrayHandle::CastAndCall` except that it internally uses :func:`viskores::cont::UnknownArrayHandle::ExtractArrayFromComponents`.

.. doxygenfunction:: viskores::cont::UnknownArrayHandle::CastAndCallWithExtractedArray

.. load-example:: UnknownArrayCallWithExtractedArray
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Calling a functor for nearly any type of array stored in an :class:`viskores::cont::UnknownArrayHandle`.


------------------------------
Mutability
------------------------------

.. index:: unknown array handle; const

One subtle feature of :class:`viskores::cont::UnknownArrayHandle` is that the class is, in principle, a pointer to an array pointer.
This means that the data in an :class:`viskores::cont::UnknownArrayHandle` is always mutable even if the class is declared ``const``.
The upshot is that you can pass output arrays as constant :class:`viskores::cont::UnknownArrayHandle` references.

.. load-example:: UnknownArrayConstOutput
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Using a ``const`` :class:`viskores::cont::UnknownArrayHandle` for a function output.

Although it seems strange, there is a good reason to allow an output :class:`viskores::cont::UnknownArrayHandle` to be ``const``.
It allows a typed :class:`viskores::cont::ArrayHandle` to be used as the argument to the function.
In this case, the compiler will automatically convert the :class:`viskores::cont::ArrayHandle` to a :class:`viskores::cont::UnknownArrayHandle`.
When C++ creates objects like this, they can only be passed a constant reference, an Rvalue reference, or by value.
So, declaring the output parameter as ``const`` :class:`viskores::cont::UnknownArrayHandle` allows it to be used for code like this.

.. load-example:: UseUnknownArrayConstOutput
   :file: GuideExampleUnknownArrayHandle.cxx
   :caption: Passing an :class:`viskores::cont::ArrayHandle` as an output :class:`viskores::cont::UnknownArrayHandle`.

Of course, you could also declare the output by value instead of by reference, but this has the same semantics with extra internal pointer management.

.. didyouknow::
   When possible, it is better to pass a :class:`viskores::cont::UnknownArrayHandle` as a constant reference (or by value) rather than a mutable reference, even if the array contents are going to be modified.
   This allows the function to support automatic conversion of an output :class:`viskores::cont::ArrayHandle`.

So if a constant :class:`viskores::cont::UnknownArrayHandle` can have its contents modified, what is the difference between a constant reference and a non-constant reference?
The difference is that the constant reference can change the array's content, but not the array itself.
If you want to do operations like doing a shallow copy or changing the underlying type of the array, a non-constant reference is needed.
