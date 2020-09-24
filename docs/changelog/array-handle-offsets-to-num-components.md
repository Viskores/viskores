# Create `ArrayHandleOffsetsToNumComponents`

`ArrayHandleOffsetsToNumComponents` is a fancy array that takes an array of
offsets and converts it to an array of the number of components for each
packed entry.

It is common in VTK-m to pack small vectors of variable sizes into a single
contiguous array. For example, cells in an explicit cell set can each have
a different amount of vertices (triangles = 3, quads = 4, tetra = 4, hexa =
8, etc.). Generally, to access items in this list, you need an array of
components in each entry and the offset for each entry. However, if you
have just the array of offsets in sorted order, you can easily derive the
number of components for each entry by subtracting adjacent entries. This
works best if the offsets array has a size that is one more than the number
of packed vectors with the first entry set to 0 and the last entry set to
the total size of the packed array (the offset to the end).

When packing data of this nature, it is common to start with an array that
is the number of components. You can convert that to an offsets array using
the `vtkm::cont::ConvertNumComponentsToOffsets` function. This will create
an offsets array with one extra entry as previously described. You can then
throw out the original number of components array and use the offsets with
`ArrayHandleOffsetsToNumComponents` to represent both the offsets and num
components while storing only one array.

This replaces the use of `ArrayHandleDecorator` in `CellSetExplicit`.
The two implementation should do the same thing, but the new
`ArrayHandleOffsetsToNumComponents` should be less complex for
compilers.
