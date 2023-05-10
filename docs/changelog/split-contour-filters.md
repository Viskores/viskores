# Split flying edges and marching cells into separate filters

The contour filter contains 2 separate implementations, Marching Cells and Flying Edges, the latter only available if the input has a `CellSetStructured<3>` and `ArrayHandleUniformPointCoordinates` for point coordinates. The compilation of this filter was lenghty and resource-heavy, because both algorithms were part of the same translation unit.

Now, this filter is separated into two new filters, `ContourFlyingEdges` and `ContourMarchingCells`, compiling more efficiently into two translation units. The `Contour` API is left unchanged. All 3 filters `Contour`, `ContourFlyingEdges` and `ContourMarchingCells` rely on a new abstract class `AbstractContour` to provide configuration and common utility functions.

Although `Contour` is still the preferred option for most cases because it selects the best implementation according to the input, `ContourMarchingCells` is usable on any kind of 3D Dataset. For now, `ContourFlyingEdges` operates only on structured uniform datasets.

Deprecate functions `GetComputeFastNormalsForStructured`, `SetComputeFastNormalsForStructured`, `GetComputeFastNormalsForUnstructured` and `GetComputeFastNormalsForUnstructured`, to use the more general `GetComputeFastNormals` and `SetComputeFastNormals` instead.

 By default, for the `Contour` filter, `GenerateNormals` is now `true`, and `ComputeFastNormals` is `false`.
