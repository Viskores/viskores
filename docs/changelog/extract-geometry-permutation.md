# Fix bug with ExtractGeometry filter

The `ExtractGeometry` filter was outputing datasets containing
`CellSetPermutation` as the representation for the cells. Although this is
technically correct and a very fast implementation, it is essentially
useless. The problem is that any downstream processing will have to know
that the data has a `CellSetPermutation`. None do (because the permutation
can be on any other cell set type, which creates an explosion of possible
cell types).

Like was done with `Threshold` a while ago, this problem is fixed by deep
copying the result into a `CellSetExplicit`. This behavior is consistent
with VTK.
