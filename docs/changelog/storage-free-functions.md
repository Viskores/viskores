# VTK-m StorageBasic now can provide or be provided a delete function

Memory that was allocated outside of VTK-m was impossible to transfer to
VTK-m as we didn't know how to free it. This is now resolved by allowing the
user to specify a free function to be called on release.

Memory that was allocated by VTK-m and Stolen by the user needed the
proper free function. When running on CUDA on hardware that supports
concurrent managed access the free function of the storage could
be cudaFree. 

To properly steal memory from VTK-m you do the following:
```cpp
  vtkm::cont::ArrayHandle<T> arrayHandle;
  //fill arrayHandle

  //you must get the free function before calling steal array
  auto free_function = arrayHandle.GetDeleteFunction();
  T* ptr = arrayHandle.StealArray();
  //use ptr


  free_function(ptr);
```
