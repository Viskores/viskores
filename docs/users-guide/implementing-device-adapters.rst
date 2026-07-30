==============================
Implementing Device Adapters
==============================

.. index::
   single: device adapter
   single: device adapter; implementing

|Viskores| comes with several implementations of device adapters so that it may be ported to a variety of platforms.
It is also possible to provide new device adapters to support yet more devices, compilers, and libraries.
A new device adapter provides a tag, a class to manage arrays in the execution environment, a collection of algorithms that run in the execution environment, and, optionally, a timer.

Most device adapters are associated with some type of device or library, and all source code related directly to that device is placed in a subdirectory of ``viskores/cont``.
For example, files associated with CUDA are in ``viskores/cont/cuda``, files associated with the Intel Threading Building Blocks (TBB) are located in ``viskores/cont/tbb``, and files associated with OpenMP are in ``viskores/cont/openmp``.
The documentation here assumes that you are adding a device adapter to the |Viskores| source code and following these file conventions.

For the purposes of discussion in this chapter, we will give a simple example of implementing a device adapter using the ``std::thread`` class provided by C++11.
We will call our device ``Cxx11Thread`` and place it in the directory ``viskores/cont/cxx11``.

By convention, the implementation of device adapters within |Viskores| is divided among internal headers named ``DeviceAdapterTag*.h``, ``DeviceAdapterRuntimeDetector*.h``, ``DeviceAdapterMemoryManager*.h``, ``RuntimeDeviceConfiguration*.h``, and ``DeviceAdapterAlgorithm*.h``.
The ``DeviceAdapter*.h`` header that most code includes is a trivial header that simply includes these other headers.
For our example ``std::thread`` device, we will create the base header at ``viskores/cont/cxx11/DeviceAdapterCxx11Thread.h``.
Its contents are the following, with minutiae such as include guards removed.

.. code-block:: cpp
   :caption: Contents of the base header for a device adapter.

   #include <viskores/cont/cxx11/internal/DeviceAdapterTagCxx11Thread.h>
   #include <viskores/cont/cxx11/internal/DeviceAdapterRuntimeDetectorCxx11Thread.h>
   #include <viskores/cont/cxx11/internal/DeviceAdapterMemoryManagerCxx11Thread.h>
   #include <viskores/cont/cxx11/internal/RuntimeDeviceConfigurationCxx11Thread.h>
   #include <viskores/cont/cxx11/internal/DeviceAdapterAlgorithmCxx11Thread.h>

The reason |Viskores| breaks up the code for its device adapters this way is that there is an interdependence between the implementation of each device adapter and the mechanism to pick a default device adapter.
Breaking up the device adapter code in this way maintains an acyclic dependence among header files.


------------------------------
Tag
------------------------------

.. index::
   single: device adapter; tag

The device adapter tag, as described in :secref:`managing-devices:Device Adapter Tag`, is a simple empty type that is used as a template parameter to identify the device adapter.
Every device adapter implementation provides one.
The device adapter tag is typically defined in an internal header file with a prefix of ``DeviceAdapterTag``.

The device adapter tag should be created with the :c:macro:`VISKORES_VALID_DEVICE_ADAPTER` macro.
This macro takes an abbreviated name that it appends to ``DeviceAdapterTag`` to make the tag structure.
It also creates support classes that allow |Viskores| to introspect the device adapter.
The macro also expects a unique integer identifier that is usually stored in a macro prefixed with ``VISKORES_DEVICE_ADAPTER_``.
The identifiers for device adapters provided by core |Viskores| are declared in ``viskores/cont/internal/DeviceAdapterTag.h``.

.. doxygendefine:: VISKORES_VALID_DEVICE_ADAPTER

If the device adapter is not being compiled, then the header file should create the tag with :c:macro:`VISKORES_INVALID_DEVICE_ADAPTER`.
This is common if the device adapter is not selected by CMake variables or the necessary libraries are not available.

.. doxygendefine:: VISKORES_INVALID_DEVICE_ADAPTER

The following example gives the implementation of our custom device adapter, which by convention would be placed in the ``viskores/cont/cxx11/internal/DeviceAdapterTagCxx11Thread.h`` header file.
This example assumes that a CMake configuration variable named ``VISKORES_ENABLE_CXX11THREAD``, which is not documented here.

.. load-example:: DeviceAdapterTagCxx11Thread
   :file: GuideExampleCustomDeviceAdapter.cxx
   :caption: Implementation of a device adapter tag.

This new device adapter tag needs to be added to :type:`viskores::cont::DeviceAdapterListCommon`, which is defined in ``viskores/cont/DeviceAdapterList.h``.
Other components of |Viskores| will use this list to write code for the device.
If you do not add the device tag to this list, then the device will not be tried when things are invoked in the execution environment, and directly specifying execution on this device will likely fail.

.. code-block:: cpp
   :name: ex:DeviceAdapterListCommon
   :caption: Modification of ``DeviceAdapterListCommon`` in ``DeviceAdapterList.h``.

   using DeviceAdapterListCommon =
     viskores::List<viskores::cont::DeviceAdapterTagCuda,
                    viskores::cont::DeviceAdapterTagTBB,
                    viskores::cont::DeviceAdapterTagOpenMP,
                    viskores::cont::DeviceAdapterTagKokkos,
                    viskores::cont::DeviceAdapterTagCxx11Thread,
                    viskores::cont::DeviceAdapterTagSerial>;

.. didyouknow::
   The order of device adapter tags in :type:`viskores::cont::DeviceAdapterListCommon` matters.
   Devices will be tried in the order listed.
   Thus, the most "preferred" devices should be listed first.
   In :numref:`ex:DeviceAdapterListCommon`, our new C++11 thread device will be used before the serial device but after the other parallel devices.

   It is OK for :type:`viskores::cont::DeviceAdapterListCommon` to contain device adapter tags for devices that are not being compiled for.
   These devices will be registered as inactive and skipped.


------------------------------
Runtime Detector
------------------------------

.. index::
   double: device adapter; runtime detector

|Viskores| defines a template named :class:`viskores::cont::DeviceAdapterRuntimeDetector` that detects whether a given device is available on the current system.
``DeviceAdapterRuntimeDetector`` has a single template argument: the device adapter tag.

.. doxygenclass:: viskores::cont::DeviceAdapterRuntimeDetector
   :members:

All device adapter implementations must create a specialization of :class:`viskores::cont::DeviceAdapterRuntimeDetector`.
They must contain a method named :func:`viskores::cont::DeviceAdapterRuntimeDetector::Exists` that returns true or false to indicate whether the device is available on the current runtime system.
For our simple C++ threading example, C++ threading is always available, even if only one such processing element exists, so our implementation simply returns true if the device has been compiled.
It determines whether the device is available using the flag ``DeviceAdapterTagCxx11Thread::IsEnabled`` at :exlineref:`ex:DeviceAdapterRuntimeDetectorCxx11Thread:ExistsFlag`, which is created by either :c:macro:`VISKORES_VALID_DEVICE_ADAPTER` or :c:macro:`VISKORES_INVALID_DEVICE_ADAPTER` as demonstrated in :numref:`ex:DeviceAdapterTagCxx11Thread`.

.. load-example:: DeviceAdapterRuntimeDetectorCxx11Thread
   :file: GuideExampleCustomDeviceAdapter.cxx
   :caption: Implementation of a ``DeviceAdapterRuntimeDetector`` specialization.


------------------------------
Memory Manager
------------------------------

.. index::
   double: device adapter; memory manager

|Viskores| defines a template named :class:`viskores::cont::internal::DeviceAdapterMemoryManager` that allocates memory on the device and copies datax.
``DeviceAdapterMemoryManager`` has a single template argument: the device adapter tag.

.. doxygenclass:: viskores::cont::internal::DeviceAdapterMemoryManager

All device adapter implementations must create a specialization of :class:`viskores::cont::internal::DeviceAdapterMemoryManager`.
This specialization must inherit from :class:`viskores::cont::internal::DeviceAdapterMemoryManagerBase`.
The memory manager allocates memory and returns it wrapped in a :class:`viskores::cont::internal::BufferInfo` object.
The superclass provides :func:`viskores::cont::internal::DeviceAdapterMemoryManagerBase::ManageArray`, which takes a raw device pointer, captured as a ``void*``, along with metadata and management functions and returns that pointer wrapped in a ``BufferInfo`` management object.

.. doxygenclass:: viskores::cont::internal::DeviceAdapterMemoryManagerBase
   :members:

.. doxygenclass:: viskores::cont::internal::BufferInfo
   :members:

A specialization of :class:`viskores::cont::internal::DeviceAdapterMemoryManager` must override the following pure virtual methods defined in the :class:`viskores::cont::internal::DeviceAdapterMemoryManagerBase` superclass.

  * :func:`viskores::cont::internal::DeviceAdapterMemoryManagerBase::GetDevice`
  * :func:`viskores::cont::internal::DeviceAdapterMemoryManagerBase::Allocate`
  * :func:`viskores::cont::internal::DeviceAdapterMemoryManagerBase::CopyHostToDevice` (two overloads)
  * :func:`viskores::cont::internal::DeviceAdapterMemoryManagerBase::CopyDeviceToHost` (two overloads)
  * :func:`viskores::cont::internal::DeviceAdapterMemoryManagerBase::CopyDeviceToDevice` (two overloads)
  * :func:`viskores::cont::internal::DeviceAdapterMemoryManagerBase::DeleteRawPointer`

If the control and execution environments share the same memory space, the execution array manager can, and should, share buffers between the host and "device" and shallow-copy data when possible.
|Viskores| provides ``viskores::cont::internal::DeviceAdapterMemoryManagerShared``, which implements a device memory manager that shares a memory space with the control environment.
In this case, the ``DeviceAdapterMemoryManager`` specialization needs to override only ``GetDevice``.
``DeviceAdapterMemoryManagerShared`` provides all other necessary overrides.

Continuing our example of a device adapter based on C++11's ``std::thread`` class, here is the implementation of ``DeviceAdapterMemoryManager``, which by convention would be placed in the ``viskores/cont/cxx11/internal/DeviceAdapterMemoryManagerCxx11Thread.h`` header file.
Because this threaded device has the same memory space as the control environment and the same memory management functions, the implementation is simplified by inheriting from :class:`viskores::cont::internal::DeviceAdapterMemoryManagerShared`, which provides the memory management for a device that shares memory with the host.

.. load-example:: DeviceAdapterMemoryManagerCxx11Thread
   :file: GuideExampleCustomDeviceAdapter.cxx
   :caption: Specialization of ``DeviceAdapterMemoryManager``.


------------------------------
Runtime Device Configuration
------------------------------

.. index::
   double: device adapter; runtime device configuration

|Viskores| defines a template named :class:`viskores::cont::internal::RuntimeDeviceConfiguration` that makes it possible to initialize runtime configuration parameters of the underlying devices.
:class:`viskores::cont::internal::RuntimeDeviceConfiguration` has a single template argument: the device adapter tag.

.. doxygenclass:: viskores::cont::internal::RuntimeDeviceConfiguration

All device adapter implementations must create a specialization of :class:`viskores::cont::internal::RuntimeDeviceConfiguration`.
This specialization must inherit from :class:`viskores::cont::internal::RuntimeDeviceConfigurationBase`.
:class:`viskores::cont::internal::RuntimeDeviceConfiguration` provides various ``Set*`` and ``Get*`` methods for setting and accessing device-specific runtime parameters.
The superclass provides :func:`viskores::cont::internal::RuntimeDeviceConfigurationBase`, which takes a :class:`viskores::cont::internal::RuntimeDeviceConfigurationOptions` argument used to set device parameters when |Viskores| is initialized.

.. doxygenclass:: viskores::cont::internal::RuntimeDeviceConfigurationBase
   :members:

.. doxygenenum:: viskores::cont::internal::RuntimeDeviceConfigReturnCode

.. doxygenclass:: viskores::cont::internal::RuntimeDeviceConfigurationOptions
   :members:

.. doxygenclass:: viskores::cont::internal::RuntimeDeviceOption
   :members:

Specializations of :class:`viskores::cont::internal::RuntimeDeviceConfiguration` must override :func:`viskores::cont::internal::RuntimeDeviceConfigurationBase::GetDevice`, which returns a :struct:`viskores::cont::DeviceAdapterId` for the device that the runtime configuration oversees.
Specializations are not required to override the other methods defined in :class:`viskores::cont::internal::RuntimeDeviceConfigurationBase`4.
These methods should be overridden only if suitable device-specific runtime parameters can be set or queried.

Continuing our example of a device adapter based on C++11's ``std::thread`` class, here is the implementation of ``RuntimeDeviceConfiguration``, which by convention would be placed in the ``viskores/cont/cxx11/internal/RuntimeDeviceConfigurationCxx11Thread.h`` header file.

.. load-example:: RuntimeDeviceConfigurationCxx11Thread
   :file: GuideExampleCustomDeviceAdapter.cxx
   :caption: Specialization of ``RuntimeDeviceConfiguration``.

.. commonerrors::
   :func:`viskores::cont::Initialize` automatically initializes the :class:`viskores::cont::internal::RuntimeDeviceConfiguration` for every available device using parsed |Viskores| command-line arguments.
   These device runtime configurations are statically managed through :class:`viskores::cont::RuntimeDeviceInformation`, which ensures that exactly one initialized instance of each :class:`viskores::cont::internal::RuntimeDeviceConfiguration` is available for each device.
   This guarantees that runtime device configuration classes cannot be initialized more than once, but it can lead to device initialization inconsistencies when code attempts to access a configuration before calling :func:`viskores::cont::Initialize`.

   When creating a new :class:`viskores::cont::internal::RuntimeDeviceConfiguration`, it is important to add an include for the new :class:`viskores::cont::DeviceAdapterRuntimeDetector` header to :class:`viskores::cont::RuntimeDeviceInformation` so that the new device is compiled correctly.
   Additionally, accessing a ``RuntimeDeviceConfiguration`` through :func:`viskores::cont::RuntimeDeviceInformation::GetRuntimeConfiguration` inside :func:`viskores::cont::DeviceAdapterRuntimeDetector::Exists` initializes the underlying device incorrectly because |Viskores| performs device-existence checks while parsing command-line arguments.

.. doxygenclass:: viskores::cont::RuntimeDeviceInformation
   :members:


------------------------------
Algorithms
------------------------------

.. index::
   double: device adapter; algorithm

A device adapter implementation must also provide a specialization of :struct:`viskores::cont::DeviceAdapterAlgorithm`, which provides the underlying implementation of the algorithms described in :chapref:`device-algorithms:Device Algorithms`.
The implementation for the device adapter algorithms is typically placed in a header file with a prefix of :struct:`viskores::cont::DeviceAdapterAlgorithm`.

.. doxygenstruct:: viskores::cont::DeviceAdapterAlgorithm
   :members:

Although there are many methods in :struct:`viskores::cont::DeviceAdapterAlgorithm`, it is seldom necessary to implement them all.
Instead, |Viskores| provides :struct:`viskores::cont::internal::DeviceAdapterAlgorithmGeneral`, which supplies generic implementations for most of the required algorithms.
By deriving the specialization of :struct:`viskores::cont::DeviceAdapterAlgorithm` from :class:`viskores::cont::internal::DeviceAdapterAlgorithmGeneral`, only :func:`viskores::cont::DeviceAdapterAlgorithm::Schedule` and :func:`viskores::cont::DeviceAdapterAlgorithm::Synchronize` need to be implemented.
All other algorithms can be derived from those.

That said, not all algorithms implemented in :struct:`viskores::cont::internal::DeviceAdapterAlgorithmGeneral` are optimized for every type of device.
Thus, it is worthwhile to provide algorithms optimized for the specific device when possible.
In particular, it is best to provide specializations for the sort, scan, and reduce algorithms.

It is standard practice to implement a specialization of :struct:`viskores::cont::DeviceAdapterAlgorithm` by having it inherit from :struct:`viskores::cont::internal::DeviceAdapterAlgorithmGeneral` and specializing those methods that are optimized for a particular system.
:struct:`viskores::cont::internal::DeviceAdapterAlgorithmGeneral` is a templated class that takes the derived algorithm and device adapter tag as template parameters.
For example, a device adapter algorithm structure named ``DeviceAdapterAlgorithm<DeviceAdapterTagFoo>`` subclasses ``DeviceAdapterAlgorithmGeneral<DeviceAdapterAlgorithm<DeviceAdapterTagFoo>, DeviceAdapterTagFoo>``.

.. doxygenstruct:: viskores::cont::internal::DeviceAdapterAlgorithmGeneral
   :members:

.. didyouknow::
   The convention of having a base class be templated on the derived class's type is known as the Curiously Recurring Template Pattern (CRTP).
   In the case of :struct:`viskores::cont::internal::DeviceAdapterAlgorithmGeneral`, |Viskores| uses this CRTP behavior to allow the general implementation of these algorithms to run :func:`viskores::cont::DeviceAdapterAlgorithm::Schedule` and other specialized algorithms in the subclass.

One point to note when implementing the :func:`viskores::cont::DeviceAdapterAlgorithm::Schedule` methods is to make sure that errors signaled in the execution environment are handled correctly.
As described in :chapref:`worklet-error-handling:Worklet Error Handling`, errors are signaled in the execution environment by calling ``RaiseError`` on a functor or worklet object.
This is handled internally by :class:`viskores::exec::internal::ErrorMessageBuffer`.
:class:`viskores::exec::internal::ErrorMessageBuffer` holds a small string buffer, which must be provided by the device adapter's :func:`viskores::cont::DeviceAdapterAlgorithm::Schedule` method.

Before :func:`viskores::cont::DeviceAdapterAlgorithm::Schedule` executes the functor it is given, it should allocate a small string array in the execution environment, initialize it to the empty string, encapsulate the array in an :class:`viskores::exec::internal::ErrorMessageBuffer` object, and set this buffer object in the functor.
When execution completes, :func:`viskores::cont::DeviceAdapterAlgorithm::Schedule` should check whether an error exists in this buffer and throw :class:`viskores::cont::ErrorExecution` if an error has been reported.

.. doxygenclass:: viskores::cont::ErrorExecution
   :members:

.. commonerrors::
   Exceptions are generally not supposed to be thrown in the execution environment, but it can happen on devices that support them.
   Nevertheless, few thread schedulers work well when an exception is thrown in them.
   Thus, when implementing adapters for devices that support exceptions, it is good practice to catch them within the thread and report them through :class:`viskores::exec::internal::ErrorMessageBuffer`.

The following example is a minimal implementation of device adapter algorithms using C++11's ``std::thread`` class.
No attempt at optimization has been made, although many optimizations are possible.
By convention, this code would be placed in the ``viskores/cont/cxx11/internal/DeviceAdapterAlgorithmCxx11Thread.h`` header file.

.. load-example:: DeviceAdapterAlgorithmCxx11Thread
   :file: GuideExampleCustomDeviceAdapter.cxx
   :caption: Minimal specialization of :struct:`viskores::cont::DeviceAdapterAlgorithm`.


------------------------------
Timer Implementation
------------------------------

.. index::
   double: device adapter; timer

The |Viskores| timer, described in :chapref:`timer:Timers`, delegates to an internal class named :class:`viskores::cont::DeviceAdapterTimerImplementation`.
The interface for this class is the same as that for :class:`viskores::cont::Timer`.
A default implementation of this templated class uses the system timer and the :func:`viskores::cont::DeviceAdapterAlgorithm::Synchronize` method in the device adapter algorithms.

However, some devices might provide alternate or better methods for implementing timers.
For example, the TBB and CUDA libraries come with high-resolution timers that have better accuracy than standard system timers.
Thus, the device adapter can optionally provide a specialization of :class:`viskores::cont::DeviceAdapterTimerImplementation`, which is typically placed in the same header file as the device adapter algorithms.

.. doxygenclass:: viskores::cont::DeviceAdapterTimerImplementation
   :members:

Continuing our example of a custom device adapter using C++11's ``std::thread`` class, we could use the default timer and it would work fine.
But C++11 also comes with a ``std::chrono`` package that contains portable time functions.
The following code demonstrates creating a custom timer for our device adapter using this package.
By convention, :class:`viskores::cont::DeviceAdapterTimerImplementation` is placed in the same header file as :struct:`viskores::cont::DeviceAdapterAlgorithm`.

.. load-example:: DeviceAdapterTimerImplementationCxx11Thread
   :file: GuideExampleCustomDeviceAdapter.cxx
   :caption: Specialization of :class:`viskores::cont::DeviceAdapterTimerImplementation`.
