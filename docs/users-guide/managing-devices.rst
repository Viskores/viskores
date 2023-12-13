==============================
Managing Devices
==============================

Multiple vendors vie to provide accelerator-type processors.
|VTKm| endeavors to support as many such architectures as possible.
Each device and device technology requires some level of code specialization, and that specialization is encapsulated in a unit called a :index:`device adapter`.

So far in :partref:`part-using:Using |VTKm|` we have been writing code that runs on a local serial CPU.
In those examples where we run a filter, |VTKm| is launching parallel execution in the execution environment.
Internally |VTKm| uses a device adapter to manage this execution.

A build of |VTKm| generally supports multiple device adapters.
In this chapter we describe how to represent and manage devices.


------------------------------
Device Adapter Tag
------------------------------

.. index::
   double: device adapter; tag

A device adapter is identified by a *device adapter tag*.
This tag, which is simply an empty struct type, is used as the template parameter for several classes in the |VTKm| control environment and causes these classes to direct their work to a particular device.
The following device adapter tags are available in |VTKm|.

.. index:: serial
.. doxygenstruct:: vtkm::cont::DeviceAdapterTagSerial
.. index:: cuda
.. doxygenstruct:: vtkm::cont::DeviceAdapterTagCuda
.. index:: OpenMP
.. doxygenstruct:: vtkm::cont::DeviceAdapterTagOpenMP
.. index:: Intel Threading Building Blocks
.. index:: TBB
.. doxygenstruct:: vtkm::cont::DeviceAdapterTagTBB
.. index:: Kokkos
.. doxygenstruct:: vtkm::cont::DeviceAdapterTagKokkos

The following example uses the tag for the Kokkos device adapter to specify a specific device for |VTKm| to use.
(Details on specifying devices in |VTKm| is provided in :secref:`managing-devices:Specifying Devices`.)

.. load-example:: SpecifyDeviceAdapter
   :file: GuideExampleRuntimeDeviceTracker.cxx
   :caption: Specifying a device using a device adapter tag.

For classes and methods that have a template argument that is expected to be a device adapter tag, the tag type can be checked with the :c:macro:`VTKM_IS_DEVICE_ADAPTER_TAG` macro to verify the type is a valid device adapter tag.
It is good practice to check unknown types with this macro to prevent further unexpected errors.

..
   Functions, methods, and classes that directly use device adapter tags are usually templated on the device adapter tag.
   This allows the function or class to be applied to any type of device specified at compile time.

   .. load-example:: DeviceTemplateArg
      :file: GuideExampleRuntimeDeviceTracker.cxx
      :caption: Specifying a device using template parameters.

   .. commonerrors::
      A device adapter tag is a class just like every other type in C++.
      Thus it is possible to accidently use a type that is not a device adapter tag when one is expected as a template argument.
      This leads to unexpected errors in strange parts of the code.
      To help identify these errors, it is good practice to use the :c:macro:`VTKM_IS_DEVICE_ADAPTER_TAG` macro to verify the type is a valid device adapter tag.
      :numref:`ex:DeviceTemplateArg` uses this macro on line 4.


------------------------------
Device Adapter Id
------------------------------

.. index::
   double: device adapter; id

Using a device adapter tag directly means that the type of device needs to be known at compile time.
To store a device adapter type at run time, one can instead use :struct:`vtkm::cont::DeviceAdapterId`.
:struct:`vtkm::cont::DeviceAdapterId` is a superclass to all the device adapter tags, and any device adapter tag can be "stored" in a :struct:`vtkm::cont::DeviceAdapterId`.
Thus, it is more common for functions and classes to use :struct:`vtkm::cont::DeviceAdapterId` then to try to track a specific device with templated code.

.. doxygenstruct:: vtkm::cont::DeviceAdapterId
   :members:

.. didyouknow::
   As a cheat, all device adapter tags actually inherit from the :struct:`vtkm::cont::DeviceAdapterId` class.
   Thus, all of these methods can be called directly on a device adapter tag.

.. commonerrors::
   Just because the :func:`vtkm::cont::DeviceAdapterId::IsValueValid` returns true that does not necessarily mean that this device is available to be run on.
   It simply means that the device is implemented in |VTKm|.
   However, that device might not be compiled, or that device might not be available on the current running system, or that device might not be enabled.
   Use the device runtime tracker described in :secref:`managing-devices:Runtime Device Tracker` to determine if a particular device can actually be used.

In addition to the provided device adapter tags listed previously, a :struct:`vtkm::cont::DeviceAdapterId` can store some special device adapter tags that do not directly specify a specific device.

.. index:: device adapter; any
.. doxygenstruct:: vtkm::cont::DeviceAdapterTagAny

.. index:: device adapter; undefined
.. doxygenstruct:: vtkm::cont::DeviceAdapterTagUndefined

.. didyouknow::
   Any device adapter tag can be used where a device adapter id is expected.
   Thus, you can use a device adapter tag whenever you want to specify a particular device and pass that to any method expecting a device id.
   Likewise, it is usually more convenient for classes and methods to manage device adapter ids rather than device adapter tag.


------------------------------
Runtime Device Tracker
------------------------------

.. index::
   single: runtime device tracker
   single: device adapter; runtime tracker

It is often the case that you are agnostic about what device |VTKm| algorithms run so long as they complete correctly and as fast as possible.
Thus, rather than directly specify a device adapter, you would like |VTKm| to try using the best available device, and if that does not work try a different device.
Because of this, there are many features in |VTKm| that behave this way.
For example, you may have noticed that running filters, as in the examples of :chapref:`running-filters:Running Filters`, you do not need to specify a device; they choose a device for you.

However, even though we often would like |VTKm| to choose a device for us, we still need a way to manage device preferences.
|VTKm| also needs a mechanism to record runtime information about what devices are available so that it does not have to continually try (and fail) to use devices that are not available at runtime.
These needs are met with the :class:`vtkm::cont::RuntimeDeviceTracker` class.
:class:`vtkm::cont::RuntimeDeviceTracker` maintains information about which devices can and should be run on.
|VTKm| maintains a :class:`vtkm::cont::RuntimeDeviceTracker` for each thread your code is operating on.
To get the runtime device for the current thread, use the :func:`vtkm::cont::GetRuntimeDeviceTracker` method.

.. doxygenfunction:: vtkm::cont::GetRuntimeDeviceTracker

.. doxygenclass:: vtkm::cont::RuntimeDeviceTracker
   :members:

.. index::
   single: runtime device tracker; scoped
   single: device adapter; scoped runtime tracker
   single: scoped device adapter


------------------------------
Specifying Devices
------------------------------

A :class:`vtkm::cont::RuntimeDeviceTracker` can be used to specify which devices to consider for a particular operation.
However, a better way to specify devices is to use the :class:`vtkm::cont::ScopedRuntimeDeviceTracker` class.
When a :class:`vtkm::cont::ScopedRuntimeDeviceTracker` is constructed, it specifies a new set of devices for |VTKm| to use.
When the :class:`vtkm::cont::ScopedRuntimeDeviceTracker` is destroyed as it leaves scope, it restores |VTKm|'s devices to those that existed when it was created.

.. doxygenclass:: vtkm::cont::ScopedRuntimeDeviceTracker
   :members:

The following example demonstrates how the :class:`vtkm::cont::ScopedRuntimeDeviceTracker` is used to force the |VTKm| operations that happen within a function to operate exclusively with the Kokkos device.

.. load-example:: ForceThreadLocalDevice
   :file: GuideExampleRuntimeDeviceTracker.cxx
   :caption: Restricting which devices |VTKm| uses per thread.

In the previous example we forced |VTKm| to use the Kokkos device.
This is the default behavior of :class:`vtkm::cont::ScopedRuntimeDeviceTracker`, but the constructor takes an optional second argument that is a value in the :enum:`vtkm::cont::RuntimeDeviceTrackerMode` to specify how modify the current device adapter list.

.. doxygenenum:: RuntimeDeviceTrackerMode

As a motivating example, let us say that we want to perform a deep copy of an array (described in :secref:`basic-array-handles:Deep Array Copies`).
However, we do not want to do the copy on a Kokkos device because we happen to know the data is not on that device and we do not want to spend the time to transfer the data to that device.
We can use a :class:`vtkm::cont::ScopedRuntimeDeviceTracker` to temporarily disable the Kokkos device for this operation.

.. load-example:: RestrictCopyDevice
   :file: GuideExampleRuntimeDeviceTracker.cxx
   :caption: Disabling a device with :class:`vtkm::cont::RuntimeDeviceTracker`.
