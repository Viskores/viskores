# Add `ArrayHandleView` fancy array

Added a new class named `ArrayHandleView` that allows you to get a subset
of an array. You use the `ArrayHandleView` by giving it a target array, a
starting index, and a length. Here is a simple example of usage:

``` cpp
vtkm::cont::ArrayHandle<vtkm::Id> sourceArray;

vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(10), sourceArray);
// sourceArray has [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

vtkm::cont::ArrayHandleView<vtkm::cont::ArrayHandle<vtkm::Id>>
  viewArray(sourceArray, 3, 5);
// viewArray has [3, 4, 5, 6, 7]
```

There is also a convenience `make_ArraHandleView` function to create view
arrays. The following makes the same view array as before.

``` cpp
auto viewArray = vtkm::cont::make_ArrayHandleView(sourceArray, 3, 5);
```
