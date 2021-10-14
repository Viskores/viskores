# Added copy methods to `UnknownArrayHandle`

`vtkm::cont::UnknownArrayHandle` now provides a set of method that allows
you to copy data from one `UnknownArrayHandle` to another. The first
method, `DeepCopyFrom`, takes a source `UnknownArrayHandle` and deep copies
the data to the called one. If the `UnknownArrayHandle` already points to a
real `ArrayHandle`, the data is copied into that `ArrayHandle`. If the
`UnknownArrayHandle` does not point to an existing `ArrayHandle`, then a
new `ArrayHandleBasic` with the same value type as the source is created
and copied into.

The second method, `CopyShallowIfPossibleFrom` behaves similarly to
`DeepCopyFrom` except that it will perform a shallow copy if possible. That
is, if the target `UnknownArrayHandle` points to an `ArrayHandle` of the
same type as the source `UnknownArrayHandle`, then a shallow copy occurs
and the underlying `ArrayHandle` will point to the source. If the types
differ, then a deep copy is performed. If the target `UnknownArrayHandle`
does not point to an `ArrayHandle`, then the behavior is the same as the
`=` operator.

One of the intentions of these new methods is to allow you to copy arrays
without using a device compiler (e.g. `nvcc`). Calling `ArrayCopy` requires
you to include the `ArrayCopy.h` header file, and that in turn requires
device adapter algorithms. These methods insulate you from these.
