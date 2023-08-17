# Support any cell type in MIR filter

Previously, the MIR filter ran a check the dimensionality of the cells in
its input data set to make sure they conformed to the algorithm. The only
real reason this was necessary is because the `MeshQuality` filter can only
check the size of either area or volume, and it has to know which one to
check. However, the `CellMeasures` filter can compute the sizes of all
types of cells simultaneously (as well as more cell types). By using this
filter, the MIR filter can skip the cell type checks and support more mesh
types.
