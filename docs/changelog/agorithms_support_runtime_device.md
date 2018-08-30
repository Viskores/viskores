# vtkm::cont::Algorithm now can be told which device to use at runtime

The `vtkm::cont::Algorithm` has been extended to support the user specifying
which device to use at runtime previously Algorithm would only use the first
enabled device, requiring users to modify the `vtkm::cont::GlobalRuntimeDeviceTracker`
if they wanted a specific device used.

To select a specific device with vtkm::cont::Algorithm pass the `vtkm::cont::DeviceAdapterId`
as the first parameter.

```cpp
vtkm::cont::ArrayHandle<double> values;


//call with no tag, will run on first enabled device
auto result = vtkm::cont::Algorithm::Reduce(values, 0.0);

//call with an explicit device tag, will only run on serial
vtkm::cont::DeviceAdapterTagSerial serial;
result = vtkm::cont::Algorithm::Reduce(serial, values, 0.0);

//call with an runtime device tag, will only run on serial
vtkm::cont::DeviceAdapterId device = serial;
result = vtkm::cont::Algorithm::Reduce(device, values, 0.0);

```
