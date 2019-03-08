# Add ability to specialize a worklet for a device

This change adds an execution signature tag named `Device` that passes
a `DeviceAdapterTag` to the worklet's parenthesis operator. This allows the
worklet to specialize its operation. This features is available in all
worklets.

The following example shows a worklet that specializes itself for the CUDA
device.

```cpp
struct DeviceSpecificWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1, Device);
  
  // Specialization for the Cuda device.
  template <typename T>
  T operator()(T x, vtkm::cont::DeviceAdapterTagCuda) const
  {
    // Special cuda implementation
  }
  
  // General implementation
  template <typename T, typename Device>
  T operator()(T x, Device) const
  {
    // General implementation
  }
};
```

## Effect on compile time and binary size

This change necessitated adding a template parameter for the device that
followed at least from the schedule all the way down. This has the
potential for duplicating several of the support methods (like
`DoWorkletInvokeFunctor`) that would otherwise have the same type. This is
especially true between the devices that run on the CPU as they should all
be sharing the same portals from `ArrayHandle`s. So the question is whether
it causes compile to take longer or cause a significant increase in
binaries.

To informally test, I first ran a clean debug compile on my Windows machine
with the serial and tbb devices. The build itself took **3 minutes, 50
seconds**. Here is a list of the binary sizes in the bin directory:

```
kmorel2 0> du -sh *.exe *.dll
200K    BenchmarkArrayTransfer_SERIAL.exe
204K    BenchmarkArrayTransfer_TBB.exe
424K    BenchmarkAtomicArray_SERIAL.exe
424K    BenchmarkAtomicArray_TBB.exe
440K    BenchmarkCopySpeeds_SERIAL.exe
580K    BenchmarkCopySpeeds_TBB.exe
4.1M    BenchmarkDeviceAdapter_SERIAL.exe
5.3M    BenchmarkDeviceAdapter_TBB.exe
7.9M    BenchmarkFieldAlgorithms_SERIAL.exe
7.9M    BenchmarkFieldAlgorithms_TBB.exe
22M     BenchmarkFilters_SERIAL.exe
22M     BenchmarkFilters_TBB.exe
276K    BenchmarkRayTracing_SERIAL.exe
276K    BenchmarkRayTracing_TBB.exe
4.4M    BenchmarkTopologyAlgorithms_SERIAL.exe
4.4M    BenchmarkTopologyAlgorithms_TBB.exe
712K    Rendering_SERIAL.exe
712K    Rendering_TBB.exe
708K    UnitTests_vtkm_cont_arg_testing.exe
1.7M    UnitTests_vtkm_cont_internal_testing.exe
13M     UnitTests_vtkm_cont_serial_testing.exe
14M     UnitTests_vtkm_cont_tbb_testing.exe
18M     UnitTests_vtkm_cont_testing.exe
13M     UnitTests_vtkm_cont_testing_mpi.exe
736K    UnitTests_vtkm_exec_arg_testing.exe
136K    UnitTests_vtkm_exec_internal_testing.exe
196K    UnitTests_vtkm_exec_serial_internal_testing.exe
196K    UnitTests_vtkm_exec_tbb_internal_testing.exe
2.0M    UnitTests_vtkm_exec_testing.exe
83M     UnitTests_vtkm_filter_testing.exe
476K    UnitTests_vtkm_internal_testing.exe
148K    UnitTests_vtkm_interop_internal_testing.exe
1.3M    UnitTests_vtkm_interop_testing.exe
2.9M    UnitTests_vtkm_io_reader_testing.exe
548K    UnitTests_vtkm_io_writer_testing.exe
792K    UnitTests_vtkm_rendering_testing.exe
3.7M    UnitTests_vtkm_testing.exe
320K    UnitTests_vtkm_worklet_internal_testing.exe
65M     UnitTests_vtkm_worklet_testing.exe
11M     vtkm_cont-1.3.dll
2.1M    vtkm_interop-1.3.dll
21M     vtkm_rendering-1.3.dll
3.9M    vtkm_worklet-1.3.dll
```

After making the singular change to the `Invocation` object to add the
`DeviceAdapterTag` as a template parameter (which should cause any extra
compile instances) the compile took **4 minuts and 5 seconds**. Here is the
new list of binaries.

```
kmorel2 0> du -sh *.exe *.dll
200K    BenchmarkArrayTransfer_SERIAL.exe
204K    BenchmarkArrayTransfer_TBB.exe
424K    BenchmarkAtomicArray_SERIAL.exe
424K    BenchmarkAtomicArray_TBB.exe
440K    BenchmarkCopySpeeds_SERIAL.exe
580K    BenchmarkCopySpeeds_TBB.exe
4.1M    BenchmarkDeviceAdapter_SERIAL.exe
5.3M    BenchmarkDeviceAdapter_TBB.exe
7.9M    BenchmarkFieldAlgorithms_SERIAL.exe
7.9M    BenchmarkFieldAlgorithms_TBB.exe
22M     BenchmarkFilters_SERIAL.exe
22M     BenchmarkFilters_TBB.exe
276K    BenchmarkRayTracing_SERIAL.exe
276K    BenchmarkRayTracing_TBB.exe
4.4M    BenchmarkTopologyAlgorithms_SERIAL.exe
4.4M    BenchmarkTopologyAlgorithms_TBB.exe
712K    Rendering_SERIAL.exe
712K    Rendering_TBB.exe
708K    UnitTests_vtkm_cont_arg_testing.exe
1.7M    UnitTests_vtkm_cont_internal_testing.exe
13M     UnitTests_vtkm_cont_serial_testing.exe
14M     UnitTests_vtkm_cont_tbb_testing.exe
19M     UnitTests_vtkm_cont_testing.exe
13M     UnitTests_vtkm_cont_testing_mpi.exe
736K    UnitTests_vtkm_exec_arg_testing.exe
136K    UnitTests_vtkm_exec_internal_testing.exe
196K    UnitTests_vtkm_exec_serial_internal_testing.exe
196K    UnitTests_vtkm_exec_tbb_internal_testing.exe
2.0M    UnitTests_vtkm_exec_testing.exe
86M     UnitTests_vtkm_filter_testing.exe
476K    UnitTests_vtkm_internal_testing.exe
148K    UnitTests_vtkm_interop_internal_testing.exe
1.3M    UnitTests_vtkm_interop_testing.exe
2.9M    UnitTests_vtkm_io_reader_testing.exe
548K    UnitTests_vtkm_io_writer_testing.exe
792K    UnitTests_vtkm_rendering_testing.exe
3.7M    UnitTests_vtkm_testing.exe
320K    UnitTests_vtkm_worklet_internal_testing.exe
68M     UnitTests_vtkm_worklet_testing.exe
11M     vtkm_cont-1.3.dll
2.1M    vtkm_interop-1.3.dll
21M     vtkm_rendering-1.3.dll
3.9M    vtkm_worklet-1.3.dll
```

So far the increase is quite negligible.
