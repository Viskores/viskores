==============================
Basic Array Handles
==============================

.. index:: array handle; basic

:chapref:`dataset:Data Sets` describes the basic data sets used by |VTKm|.
This chapter dives deeper into how |VTKm| represents data.
Ultimately, data structures like :class:`vtkm::cont::DataSet` can be broken down into arrays of numbers.
Arrays in |VTKm| are managed by a unit called an *array handle*.

An array handle, which is implemented with the :class:`vtkm::cont::ArrayHandle` class, manages an array of data that can be accessed or manipulated by |VTKm| algorithms.
It is typical to construct an array handle in the control environment to pass data to an algorithm running in the execution environment.
It is also typical for an algorithm running in the execution environment to populate an array handle, which can then be read back in the control environment.
It is also possible for an array handle to manage data created by one |VTKm| algorithm and passed to another, remaining in the execution environment the whole time and never copied to the control environment.

.. didyouknow::
   The array handle may have multiple copies of the array, one for the control environment and one for each device.
   However, depending on the device and how the array is being used, the array handle will only have one copy when possible.
   Copies between the environments are implicit and lazy.
   They are copied only when an operation needs data in an environment where the data are not.

:class:`vtkm::cont::ArrayHandle` behaves like a shared smart pointer in that when the C++ object is copied, each copy holds a reference to the same array.
These copies are reference counted so that when all copies of the :class:`vtkm::cont::ArrayHandle` are destroyed, any allocated memory is released.

.. doxygenclass:: vtkm::cont::ArrayHandle
   :members:


------------------------------
Creating Array Handles
------------------------------

:class:`vtkm::cont::ArrayHandle` is templated on the type of values being stored in the array.
There are multiple ways to create and populate an array handle.
The default :class:`vtkm::cont::ArrayHandle` constructor will create an empty array with nothing allocated in either the control or execution environment.
This is convenient for creating arrays used as the output for algorithms.

.. load-example:: CreateArrayHandle
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an :class:`vtkm::cont::ArrayHandle` for output data.

Chapter \ref{chap:AccessingAllocatingArrays} describes in detail how to allocate memory and access data in an :class:`vtkm::cont::ArrayHandle`.
However, you can use the :func:`vtkm::cont::make_ArrayHandle` function for a simplified way to create an :class:`vtkm::cont::ArrayHandle` with data.

.. todo:: Update chapter reference above. Also consider moving the access/allocation chapter earlier.

:func:`vtkm::cont::make_ArrayHandle` has many forms.
An easy form to use takes an initializer list and creates a basic :class:`vtkm::cont::ArrayHandle` with it.
This allows you to create a short :class:`vtkm::cont::ArrayHandle` from literals.

.. doxygenfunction:: vtkm::cont::make_ArrayHandle(std::initializer_list<T>&&)

.. load-example:: ArrayHandleFromInitializerList
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an :class:`vtkm::cont::ArrayHandle` from initially specified values.

One problem with creating an array from an initializer list like this is that it can be tricky to specify the exact value type of the :class:`vtkm::cont::ArrayHandle`.
The value type of the :class:`vtkm::cont::ArrayHandle` will be the same types as the literals in the initializer list, but that might not match the type you actually need.
This is particularly true for types like :type:`vtkm::Id` and :type:`vtkm::FloatDefault`, which can change depending on compile options.
To specify the exact value type to use, give that type as a template argument to the :func:`vtkm::cont::make_ArrayHandle` function.

.. load-example:: ArrayHandleFromInitializerListTyped
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating a typed :class:`vtkm::cont::ArrayHandle` from initially specified values.

Constructing an :class:`vtkm::cont::ArrayHandle` that points to a provided C array is also straightforward.
To do this, call :func:`vtkm::cont::make_ArrayHandle` with the array pointer, the number of values in the C array, and a :enum:`vtkm::CopyFlag`.
This last argument can be either :enumerator:`vtkm::CopyFlag::On` to copy the array or :enumerator:`vtkm::CopyFlag::Off` to share the provided buffer.

.. doxygenfunction:: vtkm::cont::make_ArrayHandle(const T*, vtkm::Id, vtkm::CopyFlag)

.. doxygenenum:: vtkm::CopyFlag

.. load-example:: ArrayHandleFromCArray
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an :class:`vtkm::cont::ArrayHandle` that points to a provided C array.

.. index:: vector
.. index:: std::vector

Likewise, you can use :func:`vtkm::cont::make_ArrayHandle` to transfer data from a ``std::vector`` to an :class:`vtkm::cont::ArrayHandle`.
This form of :func:`vtkm::cont::make_ArrayHandle` takes the ``std::vector`` as the first argument and a :enum:`vtkm::CopyFlag` as the second argument.

.. doxygenfunction:: vtkm::cont::make_ArrayHandle(const std::vector<T,Allocator>&, vtkm::CopyFlag)

.. load-example:: ArrayHandleFromVector
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an :class:`vtkm::cont::ArrayHandle` that points to a provided ``std::vector``.

As hinted at earlier, it is possible to send :enumerator:`vtkm::CopyFlag::On` to :func:`vtkm::cont::make_ArrayHandle` to wrap an :class:`vtkm::cont::ArrayHandle` around an existing C array or ``std::vector``.
Doing so allows you to send the data to the :class:`vtkm::cont::ArrayHandle` without copying it.
It also provides a mechanism for |VTKm| to write directly into your array.
However, *be aware* that if you change or delete the data provided, the internal state of :class:`vtkm::cont::ArrayHandle` becomes invalid and undefined behavior can ensue.
A common manifestation of this error happens when a ``std::vector`` goes out of scope.
This subtle interaction will cause the :class:`vtkm::cont::ArrayHandle` to point to an unallocated portion of the memory heap.
The following example provides an erroneous use of :class:`vtkm::cont::ArrayHandle` and some ways to fix it.

.. load-example:: ArrayOutOfScope
   :file: GuideExampleArrayHandle.cxx
   :caption: Invalidating an :class:`vtkm::cont::ArrayHandle` by letting the source ``std::vector`` leave scope.

An easy way around the problem of having an :class:`vtkm::cont::ArrayHandle`'s data going out of scope is to copy the data into the :class:`vtkm::cont::ArrayHandle`.
Simply make the :enum:`vtkm::CopyFlag` argument be :enumerator:`vtkm::CopyFlag::On` to copy the data.
This solution is shown in :exlineref:`ex:ArrayOutOfScope:CopyFlagOn`.

What if you have a ``std::vector`` that you want to pass to an :class:`vtkm::cont::ArrayHandle` and then want to only use in the :class:`vtkm::cont::ArrayHandle`?
In this case, it is wasteful to have to copy the data, but you also do not want to be responsible for keeping the ``std::vector`` in scope.
To handle this, there is a special :func:`vtkm::cont::make_ArrayHandleMove` that will move the memory out of the ``std::vector`` and into the :class:`vtkm::cont::ArrayHandle`.
:func:`vtkm::cont::make_ArrayHandleMove` takes an "rvalue" version of a ``std::vector``.
To create an "rvalue", use the ``std::move`` function provided by C++.
Once :func:`vtkm::cont::make_ArrayHandleMove` is called, the provided ``std::vector`` becomes invalid and any further access to it is undefined.
This solution is shown in :exlineref:ex:ArrayOutOfScope:MoveVector`.

.. doxygenfunction:: vtkm::cont::make_ArrayHandleMove(std::vector<T,Allocator>&&)

.. doxygenfunction:: vtkm::cont::make_ArrayHandle(std::vector<T,Allocator>&&, vtkm::CopyFlag)

.. todo:: Document moving basic C arrays somewhere.


------------------------------
Deep Array Copies
------------------------------

.. index::
   double: array handle; deep copy

As stated previously, an :class:`vtkm::cont::ArrayHandle` object behaves as a smart pointer that copies references to the data without copying the data itself.
This is clearly faster and more memory efficient than making copies of the data itself and usually the behavior desired.
However, it is sometimes the case that you need to make a separate copy of the data.

The easiest way to copy an :class:`vtkm::cont::ArrayHandle` is to use the :func:`vtkm::cont::ArrayHandle::DeepCopyFrom` method.

.. load-example:: ArrayHandleDeepCopy
   :file: GuideExampleArrayHandle.cxx
   :caption: Deep copy a :class:`vtkm::cont::ArrayHandle` of the same type.

However, the :func:`vtkm::cont::ArrayHandle::DeepCopyFrom` method only works if the two :class:`vtkm::cont::ArrayHandle` objects are the exact same type.
To simplify copying the data between :class:`vtkm::cont::ArrayHandle` objects of different types, |VTKm| comes with the :func:`vtkm::cont::ArrayCopy` convenience function defined in ``vtkm/cont/ArrayCopy.h``.
:func:`vtkm::cont::ArrayCopy` takes the array to copy from (the source) as its first argument and the array to copy to (the destination) as its second argument.
The destination array will be properly reallocated to the correct size.

.. load-example:: ArrayCopy
   :file: GuideExampleRuntimeDeviceTracker.cxx
   :caption: Using :func:`vtkm::cont::ArrayCopy`.

.. doxygenfunction:: vtkm::cont::ArrayCopy(const SourceArrayType&, DestArrayType&)

.. doxygenfunction:: vtkm::cont::ArrayCopy(const SourceArrayType&, vtkm::cont::UnknownArrayHandle&)


----------------------------------------
The Hidden Second Template Parameter
----------------------------------------

.. index::
   double: array handle; storage

We have already seen that :class:`vtkm::cont::ArrayHandle` is a templated class with the template parameter indicating the type of values stored in the array.
However, :class:`vtkm::cont::ArrayHandle` has a second hidden parameter that indicates the _storage_ of the array.
We have so far been able to ignore this second template parameter because |VTKm| will assign a default storage for us that will store the data in a basic array.

Changing the storage of an :class:`vtkm::cont::ArrayHandle` lets us do many weird and wonderful things.
We will explore these options in later chapters, but for now we can ignore this second storage template parameter.
However, there are a couple of things to note concerning the storage.

First, if the compiler gives an error concerning your use of :class:`vtkm::cont::ArrayHandle`, the compiler will report the :class:`vtkm::cont::ArrayHandle` type with not one but two template parameters.
A second template parameter of :struct:`vtkm::cont::StorageTagBasic` can be ignored.

Second, if you write a function, method, or class that is templated based on an :class:`vtkm::cont::ArrayHandle` type, it is good practice to accept an :class:`vtkm::cont::ArrayHandle` with a non-default storage type.
There are two ways to do this.
The first way is to template both the value type and the storage type.

.. load-example:: ArrayHandleParameterTemplate
   :file: GuideExampleArrayHandle.cxx
   :caption: Templating a function on an :class:`vtkm::cont::ArrayHandle`'s parameters.

The second way is to template the whole array type rather than the sub types.
If you create a template where you expect one of the parameters to be an :class:`vtkm::cont::ArrayHandle`, you should use the :c:macro:`VTKM_IS_ARRAY_HANDLE` macro to verify that the type is indeed an :class:`vtkm::cont::ArrayHandle`.

.. doxygendefine:: VTKM_IS_ARRAY_HANDLE

.. load-example:: ArrayHandleFullTemplate
   :file: GuideExampleArrayHandle.cxx
   :caption: A template parameter that should be an :class:`vtkm::cont::ArrayHandle`.


------------------------------
Mutability
------------------------------

.. index:: array handle; const

One subtle feature of :class:`vtkm::cont::ArrayHandle` is that the class is, in principle, a pointer to an array pointer.
This means that the data in an :class:`vtkm::cont::ArrayHandle` is always mutable even if the class is declared ``const``.
You can change the contents of "constant" arrays via methods like :func:`vtkm::cont::ArrayHandle::WritePortal` and :func:`vtkm::cont::ArrayHandle::PrepareForOutput`.
It is even possible to change the underlying array allocation with methods like :func:`vtkm::cont::ArrayHandle::Allocate` and :func:`vtkm::cont::ArrayHandle::ReleaseResources`.
The upshot is that you can (sometimes) pass output arrays as constant :class:`vtkm::cont::ArrayHandle` references.

So if a constant :class:`vtkm::cont::ArrayHandle` can have its contents modified, what is the difference between a constant reference and a non-constant reference?
The difference is that the constant reference can change the array's content, but not the array itself.
Basically, this means that you cannot perform shallow copies into a ``const`` :class:`vtkm::cont::ArrayHandle`.
This can be a pretty big limitation, and many of |VTKm|'s internal device algorithms still require non-constant references for outputs.
