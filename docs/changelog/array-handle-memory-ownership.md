# VTK-m ArrayHandle can now take ownership of a user allocated memory location

Previously memory that was allocated outside of VTK-m was impossible to transfer to
VTK-m as we didn't know how to free it. By extending the ArrayHandle constructors
to support a Storage object that is being moved, we can clearly express that
the ArrayHandle now owns memory it didn't allocate.

Here is an example of how this is done:
```cpp
  T* buffer = new T[100];
  auto user_free_function = [](void* ptr) { delete[] static_cast<T*>(ptr); };

  vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>
      storage(buffer, 100, user_free_function);
  vtkm::cont::ArrayHandle<T> arrayHandle(std::move(storage));
```
