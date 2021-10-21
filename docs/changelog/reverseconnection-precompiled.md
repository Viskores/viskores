# Compile reverse connectivity builder into vtkm_cont library

Because `CellSetExplicit` is a templated class, the implementation of
most of its features is part of the header files. One of the things that
was included was the code to build the reverse connectivity links. That
is, it figured out which cells were incident on each point using the
standard connections of which points comprise which cells.

Of course, building these links is non-trivial, and it used multiple
DPPs to engage the device. It meant that header had to include the
device adapter algorithms and therefore required a device compiler. We
want to minimize this where possible.

To get around this issue, a non-templated function was added to find the
reverse connections of a `CellSetExplicit`. It does this by passing in
`UnknownArrayHandle`s for the input arrays. (The output visit-points-
with-cells arrays are standard across all template instances.) The
implementation first iterates over all `CellSetExplicit` versions in
`VTKM_DEFAULT_CELL_SETS` and attempts to retrieve arrays of those types.
In the unlikely event that none of these arrays work, it copies the data
to `ArrayHandle<vtkm::Id>` and uses those.
