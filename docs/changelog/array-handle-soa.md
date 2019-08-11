# Added ArrayHandleSOA

`ArrayHandleSOA` behaves like a regular `ArrayHandle` (with a basic
storage) except that if you specify a `ValueType` of a `Vec` or a
`Vec`-like, it will actually store each component in a separate physical
array. When data are retrieved from the array, they are reconstructed into
`Vec` objects as expected.

The intention of this array type is to help cover the most common ways data
is lain out in memory. Typically, arrays of data are either an "array of
structures" like the basic storage where you have a single array of
structures (like `Vec`) or a "structure of arrays" where you have an array
of a basic type (like `float`) for each component of the data being
represented. The `ArrayHandleSOA` makes it easy to cover this second case
without creating special types.

`ArrayHandleSOA` can be constructed from a collection of `ArrayHandle` with
basic storage. This allows you to construct `Vec` arrays from components
without deep copies.

``` cpp
std::vector<vtkm::Float32> accel0;
std::vector<vtkm::Float32> accel1;
std::vector<vtkm::Float32> accel2;

// Let's say accel arrays are set to some field of acceleration vectors by
// some other software.

vtkm::cont::ArrayHandle<vtkm::Float32> accelHandle0 = vtkm::cont::make_ArrayHandle(accel0);
vtkm::cont::ArrayHandle<vtkm::Float32> accelHandle1 = vtkm::cont::make_ArrayHandle(accel1);
vtkm::cont::ArrayHandle<vtkm::Float32> accelHandle2 = vtkm::cont::make_ArrayHandle(accel2);

vtkm::cont::ArrayHandleSOA<vtkm::Vec3f_32> accel = { accelHandle0, accelHandle1, accelHandle2 };
```

Also provided are constructors and versions of `make_ArrayHandleSOA` that
take `std::vector` or C arrays as either initializer lists or variable
arguments.

``` cpp
std::vector<vtkm::Float32> accel0;
std::vector<vtkm::Float32> accel1;
std::vector<vtkm::Float32> accel2;

// Let's say accel arrays are set to some field of acceleration vectors by
// some other software.

vtkm::cont::ArrayHandleSOA<vtkm::Vec3f_32> accel = { accel0, accel1, accel2 };
```

However, setting arrays is a little awkward because you also have to
specify the length. This is done either outside the initializer list or as
the first argument.

``` cpp
vtkm::cont::make_ArrayHandleSOA({ array0, array1, array2 }, ARRAY_SIZE);
```

``` cpp
vtkm::cont::make_ArrayHandleSOA(ARRAY_SIZE, array0, array1, array2);
```
