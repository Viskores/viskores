# vtkm::Vec const& operator[] is now `constexpr`


This was done to allow for developers to write normal operations on vtkm::Vec but have
the resolved at compile time, allowing for both readible code and no runtime cost.

Now you can do things such as:
```cxx
  constexpr vtkm::Id2 dims(16,16);
  constexpr vtkm::Float64 dx = vtkm::Float64(4.0 * vtkm::Pi()) / vtkm::Float64(dims[0] - 1);
```
