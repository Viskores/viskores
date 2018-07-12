# VTK-m VirtualObjectHandle can transfer to a device using runtime `DeviceAdapterId` value

Previously `VirtualObjectHandle` required the caller to know a compile time device adapter tag to 
transfer data. This was problematic since in parts of VTK-m you would only have the runtime
`vtkm::cont::DeviceAdapterId` value of the desired device. To than transfer the 
`VirtualObjectHandle` you would have to call `FindDeviceAdapterTagAndCall`. All this extra
work was unneeded as `VirtualObjectHandle` internally was immediately converting from
a compile time type to a runtime value.


Here is an example of how you can now transfer a `VirtualObjectHandle` to a device using
a runtime value:
```cpp

template<typename BaseType>
const BaseType* moveToDevice(VirtualObjectHandle<BaseType>& handle,
                      vtkm::cont::vtkm::cont::DeviceAdapterId deviceId)
{
  return handle.PrepareForExecution(deviceId);
}
```
