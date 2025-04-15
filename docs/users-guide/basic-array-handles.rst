==============================
Basic Array Handles
==============================

.. index:: array handle; basic

:chapref:`dataset:Data Sets` describes the basic data sets used by |Viskores|.
This chapter dives deeper into how |Viskores| represents data.
Ultimately, data structures like :class:`viskores::cont::DataSet` can be broken down into arrays of numbers.
Arrays in |Viskores| are managed by a unit called an *array handle*.

An array handle, which is implemented with the :class:`viskores::cont::ArrayHandle` class, manages an array of data that can be accessed or manipulated by |Viskores| algorithms.
It is typical to construct an array handle in the control environment to pass data to an algorithm running in the execution environment.
It is also typical for an algorithm running in the execution environment to populate an array handle, which can then be read back in the control environment.
It is also possible for an array handle to manage data created by one |Viskores| algorithm and passed to another, remaining in the execution environment the whole time and never copied to the control environment.

.. didyouknow::
   The array handle may have multiple copies of the array, one for the control environment and one for each device.
   However, depending on the device and how the array is being used, the array handle will only have one copy when possible.
   Copies between the environments are implicit and lazy.
   They are copied only when an operation needs data in an environment where the data are not.

:class:`viskores::cont::ArrayHandle` behaves like a shared smart pointer in that when the C++ object is copied, each copy holds a reference to the same array.
These copies are reference counted so that when all copies of the :class:`viskores::cont::ArrayHandle` are destroyed, any allocated memory is released.

.. doxygenclass:: viskores::cont::ArrayHandle
   :members:


------------------------------
Creating Array Handles
------------------------------

:class:`viskores::cont::ArrayHandle` is templated on the type of values being stored in the array.
There are multiple ways to create and populate an array handle.
The default :class:`viskores::cont::ArrayHandle` constructor will create an empty array with nothing allocated in either the control or execution environment.
This is convenient for creating arrays used as the output for algorithms.

.. load-example:: CreateArrayHandle
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an :class:`viskores::cont::ArrayHandle` for output data.

There are times when you will wish to create a :class:`viskores::cont::ArrayHandle` populated with existing data.
This can be done with the :func:`viskores::cont::make_ArrayHandle` function.
:func:`viskores::cont::make_ArrayHandle` has many forms.
An easy form to use takes an initializer list and creates a basic :class:`viskores::cont::ArrayHandle` with it.
This allows you to create a short :class:`viskores::cont::ArrayHandle` from literals.

.. doxygenfunction:: viskores::cont::make_ArrayHandle(std::initializer_list<T>&&)

.. load-example:: ArrayHandleFromInitializerList
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an :class:`viskores::cont::ArrayHandle` from initially specified values.

One problem with creating an array from an initializer list like this is that it can be tricky to specify the exact value type of the :class:`viskores::cont::ArrayHandle`.
The value type of the :class:`viskores::cont::ArrayHandle` will be the same types as the literals in the initializer list, but that might not match the type you actually need.
This is particularly true for types like :type:`viskores::Id` and :type:`viskores::FloatDefault`, which can change depending on compile options.
To specify the exact value type to use, give that type as a template argument to the :func:`viskores::cont::make_ArrayHandle` function.

.. load-example:: ArrayHandleFromInitializerListTyped
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating a typed :class:`viskores::cont::ArrayHandle` from initially specified values.

Constructing an :class:`viskores::cont::ArrayHandle` that points to a provided C array is also straightforward.
To do this, call :func:`viskores::cont::make_ArrayHandle` with the array pointer, the number of values in the C array, and a :enum:`viskores::CopyFlag`.
This last argument can be either :enumerator:`viskores::CopyFlag::On` to copy the array or :enumerator:`viskores::CopyFlag::Off` to share the provided buffer.

.. doxygenfunction:: viskores::cont::make_ArrayHandle(const T*, viskores::Id, viskores::CopyFlag)

.. doxygenenum:: viskores::CopyFlag

.. load-example:: ArrayHandleFromCArray
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an :class:`viskores::cont::ArrayHandle` that points to a provided C array.

.. index:: vector
.. index:: std::vector

Likewise, you can use :func:`viskores::cont::make_ArrayHandle` to transfer data from a ``std::vector`` to an :class:`viskores::cont::ArrayHandle`.
This form of :func:`viskores::cont::make_ArrayHandle` takes the ``std::vector`` as the first argument and a :enum:`viskores::CopyFlag` as the second argument.

.. doxygenfunction:: viskores::cont::make_ArrayHandle(const std::vector<T,Allocator>&, viskores::CopyFlag)

.. load-example:: ArrayHandleFromVector
   :file: GuideExampleArrayHandle.cxx
   :caption: Creating an :class:`viskores::cont::ArrayHandle` that points to a provided ``std::vector``.

As hinted at earlier, it is possible to send :enumerator:`viskores::CopyFlag::On` to :func:`viskores::cont::make_ArrayHandle` to wrap an :class:`viskores::cont::ArrayHandle` around an existing C array or ``std::vector``.
Doing so allows you to send the data to the :class:`viskores::cont::ArrayHandle` without copying it.
It also provides a mechanism for |Viskores| to write directly into your array.
However, *be aware* that if you change or delete the data provided, the internal state of :class:`viskores::cont::ArrayHandle` becomes invalid and undefined behavior can ensue.
A common manifestation of this error happens when a ``std::vector`` goes out of scope.
This subtle interaction will cause the :class:`viskores::cont::ArrayHandle` to point to an unallocated portion of the memory heap.
The following example provides an erroneous use of :class:`viskores::cont::ArrayHandle` and some ways to fix it.

.. load-example:: ArrayOutOfScope
   :file: GuideExampleArrayHandle.cxx
   :caption: Invalidating an :class:`viskores::cont::ArrayHandle` by letting the source ``std::vector`` leave scope.

An easy way around the problem of having an :class:`viskores::cont::ArrayHandle`'s data going out of scope is to copy the data into the :class:`viskores::cont::ArrayHandle`.
Simply make the :enum:`viskores::CopyFlag` argument be :enumerator:`viskores::CopyFlag::On` to copy the data.
This solution is shown in :exlineref:`ex:ArrayOutOfScope:CopyFlagOn`.

What if you have a ``std::vector`` that you want to pass to an :class:`viskores::cont::ArrayHandle` and then want to only use in the :class:`viskores::cont::ArrayHandle`?
In this case, it is wasteful to have to copy the data, but you also do not want to be responsible for keeping the ``std::vector`` in scope.
To handle this, there is a special :func:`viskores::cont::make_ArrayHandleMove` that will move the memory out of the ``std::vector`` and into the :class:`viskores::cont::ArrayHandle`.
:func:`viskores::cont::make_ArrayHandleMove` takes an "rvalue" version of a ``std::vector``.
To create an "rvalue", use the ``std::move`` function provided by C++.
Once :func:`viskores::cont::make_ArrayHandleMove` is called, the provided ``std::vector`` becomes invalid and any further access to it is undefined.
This solution is shown in :exlineref:ex:ArrayOutOfScope:MoveVector`.

.. doxygenfunction:: viskores::cont::make_ArrayHandleMove(std::vector<T,Allocator>&&)

.. doxygenfunction:: viskores::cont::make_ArrayHandle(std::vector<T,Allocator>&&, viskores::CopyFlag)

.. todo:: Document moving basic C arrays somewhere.


------------------------------
Allocating
------------------------------

.. index::
   double: array handle; allocate

:class:`viskores::cont::ArrayHandle` is capable of allocating its own memory.
The most straightforward way to allocate memory is to call the :func:`viskores::cont::ArrayHandle::Allocate` method.
The :func:`viskores::cont::ArrayHandle::Allocate` method takes a single argument, which is the number of elements to make the array.

.. load-example:: ArrayHandleAllocate
   :file: GuideExampleArrayHandle.cxx
   :caption: Allocating an :class:`viskores::cont::ArrayHandle`.

By default when you :func:`viskores::cont::ArrayHandle::Allocate` an array, it potentially destroys any existing data in it.
However, there are cases where you wish to grow or shrink an array while preserving the existing data.
To preserve the existing data when allocating an array, pass :enumerator:`viskores::CopyFlag::On` as an optional second argument.

.. load-example:: ArrayHandleReallocate
   :file: GuideExampleArrayHandle.cxx
   :caption: Resizing an :class:`viskores::cont::ArrayHandle`.

It is also possible to initialize new values in an allocated :class:`viskores::cont::ArrayHandle` by using the :func:`viskores::cont::ArrayHandle::AllocateAndFill` method.

.. didyouknow::
   The ability to allocate memory is a key difference between :class:`viskores::cont::ArrayHandle` and many other common forms of smart pointers.
   When one :class:`viskores::cont::ArrayHandle` allocates new memory, all other :class:`viskores::cont::ArrayHandle`'s pointing to the same managed memory get the newly allocated memory.
   This feature makes it possible to pass a :class:`viskores::cont::ArrayHandle` to a method to be reallocated and filled without worrying about C++ details on how to reference the :class:`viskores::cont::ArrayHandle` object itself.


------------------------------
Deep Array Copies
------------------------------

.. index::
   double: array handle; deep copy

As stated previously, an :class:`viskores::cont::ArrayHandle` object behaves as a smart pointer that copies references to the data without copying the data itself.
This is clearly faster and more memory efficient than making copies of the data itself and usually the behavior desired.
However, it is sometimes the case that you need to make a separate copy of the data.

The easiest way to copy an :class:`viskores::cont::ArrayHandle` is to use the :func:`viskores::cont::ArrayHandle::DeepCopyFrom` method.

.. load-example:: ArrayHandleDeepCopy
   :file: GuideExampleArrayHandle.cxx
   :caption: Deep copy a :class:`viskores::cont::ArrayHandle` of the same type.

However, the :func:`viskores::cont::ArrayHandle::DeepCopyFrom` method only works if the two :class:`viskores::cont::ArrayHandle` objects are the exact same type.
To simplify copying the data between :class:`viskores::cont::ArrayHandle` objects of different types, |Viskores| comes with the :func:`viskores::cont::ArrayCopy` convenience function defined in ``viskores/cont/ArrayCopy.h``.
:func:`viskores::cont::ArrayCopy` takes the array to copy from (the source) as its first argument and the array to copy to (the destination) as its second argument.
The destination array will be properly reallocated to the correct size.

.. load-example:: ArrayCopy
   :file: GuideExampleRuntimeDeviceTracker.cxx
   :caption: Using :func:`viskores::cont::ArrayCopy`.

.. doxygenfunction:: viskores::cont::ArrayCopy(const SourceArrayType&, DestArrayType&)

.. doxygenfunction:: viskores::cont::ArrayCopy(const SourceArrayType&, viskores::cont::UnknownArrayHandle&)


------------------------------
Array Portals
------------------------------

.. index::
   single: array portal
   single: array handle; array portal

The :class:`viskores::cont::ArrayHandle` class does not provide direct access to the data in the array.
This is because the control and access to arrays is often in different parts of the code in |Viskores|.
To get direct access to the data, you must retrieve an *array portal* to the array.
There is no single :class:`ArrayPortal` class declared, but the structure of all such classes has the following members.

.. cpp:class:: ArrayPortal

   A class that provides access to the data in an array.
   Each :class:`viskores::cont::ArrayHandle` type defines its own array portal.

.. cpp:type:: T ArrayPortal::ValueType

   The type for each item in the array.

.. cpp:function:: viskores::Id ArrayPortal::GetNumberOfValues() const

   Returns the number of entries in the array.

.. cpp:function:: ArrayPortal::ValueType ArrayPortal::Get(viskores::Id index) const

   Returns the value in the array at the given index.

.. cpp:function:: void ArrayPortal::Set(viskores::Id index, const ArrayPortal::ValueType& value) const

   Sets the entry at the given index of the array to the provided value.

A :class:`viskores::cont::ArrayHandle` provides its own array portal of an internal type.
The correct type for the array portal is :type:`viskores::cont::ArrayHandle::ReadPortalType` for read-only access and :type:`viskores::cont::ArrayHandle::WritePortalType` for read-write access.

:class:`viskores::cont::ArrayHandle` provides the methods :func:`viskores::cont::ArrayHandle::ReadPortal` and :func:`viskores::cont::ArrayHandle::WritePortal` to get the associated array portal objects to access the data in the control environment.
These methods also have the side effect of refreshing the control environment copy of the data as if you called :func:`viskores::cont::ArrayHandle::SyncControlArray`.
Be aware that calling :func:`viskores::cont::ArrayHandle::WritePortal` will invalidate any copy in the execution environment, meaning that any subsequent use will cause the data to be copied back again.

.. load-example:: ArrayHandlePopulate
   :file: GuideExampleArrayHandle.cxx
   :caption: Populating a :class:`viskores::cont::ArrayHandle`.

.. didyouknow::
   Most operations on arrays in |Viskores| should really be done in the execution environment.
   Keep in mind that whenever doing an operation using a control array portal, that operation will likely be slow for large arrays.
   However, some operations, like performing file I/O, make sense in the control environment.

.. commonerrors::
   The portal returned from :func:`viskores::cont::ArrayHandle::ReadPortal` or :func:`viskores::cont::ArrayHandle::WritePortal` is only good as long as the data in the :class:`viskores::cont::ArrayHandle` are not moved or reallocated.
   For example, if you call :func:`viskores::cont::ArrayHandle::Allocate`, any previously created array portals are likely to become invalid, and using them will result in undefined behavior.
   Thus, you should keep portals only as long as is necessary to complete an operation.

|Viskores| provides a pair of functions, :func:`viskores::cont::ArrayPortalToIteratorBegin` and :func:`viskores::cont::ArrayPortalToIterationEnd`, to convert an :class:`ArrayPortal` into a C++ STL iterator.
This makes it easy to operate on |Viskores| arrays like other C++ STL containers, but keep in mind this will all be done in serial on the host processor.

.. doxygenfunction:: viskores::cont::ArrayPortalToIteratorBegin
.. doxygenfunction:: viskores::cont::ArrayPortalToIteratorEnd

.. load-example:: ControlPortals
   :file: GuideExampleArrayHandle.cxx
   :caption: Using portals as C++ iterators.


----------------------------------------
The Hidden Second Template Parameter
----------------------------------------

.. index::
   double: array handle; storage

We have already seen that :class:`viskores::cont::ArrayHandle` is a templated class with the template parameter indicating the type of values stored in the array.
However, :class:`viskores::cont::ArrayHandle` has a second hidden parameter that indicates the _storage_ of the array.
We have so far been able to ignore this second template parameter because |Viskores| will assign a default storage for us that will store the data in a basic array.

Changing the storage of an :class:`viskores::cont::ArrayHandle` lets us do many weird and wonderful things.
We will explore these options in later chapters, but for now we can ignore this second storage template parameter.
However, there are a couple of things to note concerning the storage.

First, if the compiler gives an error concerning your use of :class:`viskores::cont::ArrayHandle`, the compiler will report the :class:`viskores::cont::ArrayHandle` type with not one but two template parameters.
A second template parameter of :struct:`viskores::cont::StorageTagBasic` can be ignored.

Second, if you write a function, method, or class that is templated based on an :class:`viskores::cont::ArrayHandle` type, it is good practice to accept an :class:`viskores::cont::ArrayHandle` with a non-default storage type.
There are two ways to do this.
The first way is to template both the value type and the storage type.

.. load-example:: ArrayHandleParameterTemplate
   :file: GuideExampleArrayHandle.cxx
   :caption: Templating a function on an :class:`viskores::cont::ArrayHandle`'s parameters.

The second way is to template the whole array type rather than the sub types.
If you create a template where you expect one of the parameters to be an :class:`viskores::cont::ArrayHandle`, you should use the :c:macro:`VISKORES_IS_ARRAY_HANDLE` macro to verify that the type is indeed an :class:`viskores::cont::ArrayHandle`.

.. doxygendefine:: VISKORES_IS_ARRAY_HANDLE

.. load-example:: ArrayHandleFullTemplate
   :file: GuideExampleArrayHandle.cxx
   :caption: A template parameter that should be an :class:`viskores::cont::ArrayHandle`.


------------------------------
Mutability
------------------------------

.. index:: array handle; const

One subtle feature of :class:`viskores::cont::ArrayHandle` is that the class is, in principle, a pointer to an array pointer.
This means that the data in an :class:`viskores::cont::ArrayHandle` is always mutable even if the class is declared ``const``.
You can change the contents of "constant" arrays via methods like :func:`viskores::cont::ArrayHandle::WritePortal` and :func:`viskores::cont::ArrayHandle::PrepareForOutput`.
It is even possible to change the underlying array allocation with methods like :func:`viskores::cont::ArrayHandle::Allocate` and :func:`viskores::cont::ArrayHandle::ReleaseResources`.
The upshot is that you can (sometimes) pass output arrays as constant :class:`viskores::cont::ArrayHandle` references.

So if a constant :class:`viskores::cont::ArrayHandle` can have its contents modified, what is the difference between a constant reference and a non-constant reference?
The difference is that the constant reference can change the array's content, but not the array itself.
Basically, this means that you cannot perform shallow copies into a ``const`` :class:`viskores::cont::ArrayHandle`.
This can be a pretty big limitation, and many of |Viskores|'s internal device algorithms still require non-constant references for outputs.
