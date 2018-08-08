# TryExecuteOnDevice allows for runtime selection of which device to execute on

VTK-m now offers `vtkm::cont::TryExecuteOnDevice` to allow for the user to select 
which device to execute a function on at runtime. The original `vtkm::cont::TryExecute`
used the first valid device, which meant users had to modify the runtime state
through the `RuntimeTracker` which was verbose and unwieldy. 

Here is an example of how you can execute a function on the device that an array handle was last executed
on:
```cpp

struct ArrayCopyFunctor
{
  template <typename Device, typename InArray, typename OutArray>
  VTKM_CONT bool operator()(Device, const InArray& src, OutArray& dest)
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(src, dest);    
    return true;
  }
};

template<typename T, typename InStorage, typename OutStorage>
void SmartCopy(const vtkm::cont::ArrayHandle<T, InStorage>& src, vtkm::cont::ArrayHandle<T, OutStorage>& dest)
{
  bool success = vtkm::cont::TryExecuteOnDevice(devId, ArrayCopyFunctor(), src, dest);
  if (!success)
  {
    vtkm::cont::TryExecute(ArrayCopyFunctor(), src, dest);
  }
}
```
