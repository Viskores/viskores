# Add ArrayGetValues to retrieve a subset of ArrayHandle values from a device.

An algorithm will often want to pull just a single value (or small subset of
values) back from a device to check the results of a computation. Previously,
there was no easy way to do this, and algorithm developers would often
transfer vast quantities of data back to the host just to check a single value.

The new `vtkm::cont::ArrayGetValue` and `vtkm::cont::ArrayGetValues` functions
simplify this operations and provide a method to just retrieve a portion of an
array.

This utility provides several convenient overloads:

A single id may be passed into ArrayGetValue, or multiple ids may be specified
to ArrayGetValues as an ArrayHandle<vtkm::Id>, a std::vector<vtkm::Id>, a
c-array (pointer and size), or as a brace-enclosed initializer list.

The single result from ArrayGetValue may be returned or written to an output
argument. Multiple results from ArrayGetValues may be returned as an
std::vector<T>, or written to an output argument as an ArrayHandle<T> or a
std::vector<T>.

Examples:

```
vtkm::cont::ArrayHandle<T> data = ...;

// Fetch the first value in an array handle:
T firstVal = vtkm::cont::ArrayGetValue(0, data);

// Fetch the first and third values in an array handle:
std::vector<T> firstAndThird = vtkm::cont::ArrayGetValues({0, 2}, data);

// Fetch the first and last values in an array handle:
std::vector<T> firstAndLast =
    vtkm::cont::ArrayGetValues({0, data.GetNumberOfValues() - 1}, data);

// Fetch the first 4 values into an array handle:
const std::vector<vtkm::Id> ids{0, 1, 2, 3};
vtkm::cont::ArrayHandle<T> firstFour;
vtkm::cont::ArrayGetValues(ids, data, firstFour);
```
