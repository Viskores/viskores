# Redesign Runtime Device Tracking

The device tracking infrastructure in VTK-m has been redesigned to
remove multiple redundant codes paths and to simplify reasoning
about around what an instance of RuntimeDeviceTracker will modify.

`vtkm::cont::RuntimeDeviceTracker` tracks runtime information on
a per-user thread basis. This is done to allow multiple calling
threads to use different vtk-m backends such as seen in this
example:

```cpp
  vtkm::cont::DeviceAdapterTagCuda cuda;
  vtkm::cont::DeviceAdapterTagOpenMP openmp;
  { // thread 1
    auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
    tracker->ForceDevice(cuda);
    vtkm::worklet::Invoker invoke;
    invoke(LightTask{}, input, output);
    vtkm::cont::Algorithm::Sort(output);
    invoke(HeavyTask{}, output);
  }

 { // thread 2
    auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
    tracker->ForceDevice(openmp);
    vtkm::worklet::Invoker invoke;
    invoke(LightTask{}, input, output);
    vtkm::cont::Algorithm::Sort(output);
    invoke(HeavyTask{}, output);
  }
```

While this address the ability for threads to specify what
device they should run on. It doesn't make it easy to toggle
the status of a device in a programmatic way, for example
the following block forces execution to only occur on 
`cuda` and doesn't restore previous active devices after 

```cpp  
  {
  vtkm::cont::DeviceAdapterTagCuda cuda;
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker->ForceDevice(cuda);
  vtkm::worklet::Invoker invoke;
  invoke(LightTask{}, input, output);
  } 
  //openmp/tbb/... still inactive
```

To resolve those issues we have `vtkm::cont::ScopedRuntimeDeviceTracker` which
has the same interface as `vtkm::cont::RuntimeDeviceTracker` but additionally
resets any per-user thread modifications when it goes out of scope. So by
switching over the previous example to use `ScopedRuntimeDeviceTracker` we
correctly restore the threads `RuntimeDeviceTracker` state when `tracker`
goes out of scope.
```cpp  
  {
  vtkm::cont::DeviceAdapterTagCuda cuda;
  vtkm::cont::ScopedRuntimeDeviceTracker tracker(cuda);
  vtkm::worklet::Invoker invoke;
  invoke(LightTask{}, input, output);
  } 
  //openmp/tbb/... are now again active
```

The  `vtkm::cont::ScopedRuntimeDeviceTracker` is not limited to forcing
execution to occur on a single device. When constructed it can either force
execution to a device, disable a device or enable a device. These options
also work with the `DeviceAdapterTagAny`.


```cpp  
  {
  //enable all devices 
  vtkm::cont::DeviceAdapterTagAny any;
  vtkm::cont::ScopedRuntimeDeviceTracker tracker(any, 
                                                 vtkm::cont::RuntimeDeviceTrackerMode::Enable);
  ...
  }

  {
  //disable only cuda
  vtkm::cont::DeviceAdapterTagCuda cuda;
  vtkm::cont::ScopedRuntimeDeviceTracker tracker(cuda, 
                                                 vtkm::cont::RuntimeDeviceTrackerMode::Disable);

  ...
  }
```
