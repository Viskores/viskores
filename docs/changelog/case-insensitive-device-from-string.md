# VTK-m  `vtkm::cont::DeviceAdapterId` construction from string are now case-insensitive

You can now construct a `vtkm::cont::DeviceAdapterId` from a string no matter
the case of it. The following all will construct the same `vtkm::cont::DeviceAdapterId`.

```cpp
vtkm::cont::DeviceAdapterId id1 = vtkm::cont::make_DeviceAdapterId("cuda");
vtkm::cont::DeviceAdapterId id2 = vtkm::cont::make_DeviceAdapterId("CUDA");
vtkm::cont::DeviceAdapterId id3 = vtkm::cont::make_DeviceAdapterId("Cuda");

auto& tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker();
vtkm::cont::DeviceAdapterId id4 = tracker.GetDeviceAdapterId("cuda");
vtkm::cont::DeviceAdapterId id5 = tracker.GetDeviceAdapterId("CUDA");
vtkm::cont::DeviceAdapterId id6 = tracker.GetDeviceAdapterId("Cuda");
