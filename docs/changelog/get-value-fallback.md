# Better fallback for ArrayGetValue

To avoid having to use a device compiler every time you wish to use
`ArrayGetValue`, the actual implementation is compiled into the `vtkm_cont`
library. To allow this to work for all the templated versions of
`ArrayHandle`, the implementation uses the extract component features of
`UnknownArrayHandle`. This works for most common arrays, but not all
arrays.

For arrays that cannot be directly represented by an `ArrayHandleStride`,
the fallback is bad. The entire array has to be pulled to the host and then
copied serially to a basic array.

For `ArrayGetValue`, this is just silly. So, for arrays that cannot be
simply represented by `ArrayHandleStride`, make a fallback that just uses
`ReadPortal` to get the data. Often this is not the most efficient method,
but it is better than the current alternative.
