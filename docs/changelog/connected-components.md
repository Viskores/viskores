# Add connected component worklets and filters

We have added the `ImageConnectivity` and `CellSetConnectivity` worklets and
the corresponding filters to identify connected components in DataSet. The ImageConnectivity
identify connected components in CellSetStructured, based on same field value of neighboring
cells and the CellSetConnective identify connected components based on cell connectivity.
Currently Moore neighborhood (i.e. 8 neighboring pixels for 2D and 27 neighboring pixels
for 3D) is used for ImageConnectivity. For CellSetConnectivity, neighborhood is defined
as cells sharing a common edge.
