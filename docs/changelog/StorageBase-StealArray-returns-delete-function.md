## `StorageBasic` StealArray() now provides delete function to new owner

Memory that is stolen from VTK-m has to be freed correctly. This is required
as the memory could have been allocated with `new`, `malloc` or even `cudaMallocManaged`.

Previously it was very easy to transfer ownership of memory out of VTK-m and
either fail to capture the free function, or ask for it after the transfer
operation which would return a nullptr. Now stealing an array also
provides the free function reducing one source of memory leaks.

To properly steal memory from VTK-m you do the following:
```cpp
  vtkm::cont::ArrayHandle<T> arrayHandle;
  
  ...
  
  auto* stolen = arrayHandle.StealArray();
  T* ptr = stolen.first;
  auto free_function = stolen.second;
  
  ...

  free_function(ptr);
```
