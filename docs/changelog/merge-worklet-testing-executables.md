Merge worklet testing executables into a device dependent shared library

VTK-m has been updated to replace old per device worklet testing executables with a device
dependent shared library so that it's able to accept a device adapter
at runtime.
Meanwhile, it updates the testing infrastructure APIs. vtkm::cont::testing::Run
function would call ForceDevice when needed and if users need the device
adapter info at runtime, RunOnDevice function would pass the adapter into the functor.

Optional Parser is bumped from 1.3 to 1.7.

