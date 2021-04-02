# `vtkm::cont::internal::Buffer` now can have ownership transferred

Memory once transferred to `Buffer` always had to be managed by VTK-m. This is problematic
for applications that needed VTK-m to allocate memory, but have the memory ownership
be longer than VTK-m.

`Buffer::TakeHostBufferOwnership` allows for easy transfer ownership of memory out of VTK-m.
When taking ownership of an VTK-m buffer you are provided the following information:

- Memory: A `void*` pointer to the array
- Container: A `void*` pointer used to free the memory. This is necessary to support cases such as allocations transferred into VTK-m from a `std::vector`.
- Delete: The function to call to actually delete the transferred memory
- Reallocate: The function to call to re-allocate the transferred memory. This will throw an exception if users try
to reallocate a buffer that was 'view' only
- Size: The size in number of elements of the array 

 
To properly steal memory from VTK-m you do the following:
```cpp
  vtkm::cont::ArrayHandle<T> arrayHandle;

  ...

  auto stolen = arrayHandle.GetBuffers()->TakeHostBufferOwnership();
    
  ...

  stolen.Delete(stolen.Container);
```
