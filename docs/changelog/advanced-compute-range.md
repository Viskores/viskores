# New features for computing array ranges

ArrayRangeCompute has been update to support more features that are present
in VTK and ParaView.

New overloads for `ArrayRangeCompute` have been added:
1. Takes a boolean parameter, `computeFiniteRange`, that specifies
whether to compute only the finite range by ignoring any non-finite values (+/-inf)
in the array.

2. Takes a `maskArray` parameter of type `vtkm::cont::ArrayHandle<vtkm::UInt8>`.
The mask array must contain the same number of elements as the input array.
A value in the input array is treated as masked off if the
corresponding value in the mask array is non-zero. Masked off values are ignored
in the range computation.

A new function `ArrayRangeComputeMagnitude` has been added. If the input array
has multiple components, this function computes the range of the magnitude of
the values of the array. Nested Vecs are treated as flat. A single `Range` object
is returned containing the result. `ArrayRangeComputMagnitude` also has similar
overloads as `ArrayRangeCompute`.
