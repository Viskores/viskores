# Reorganization of `io` directory

The `vtkm/io` directory has been flattened.
Namely, the files in `vtkm/io/reader` and `vtkm/io/writer` have been moved up into `vtkm/io`,
with the associated changes in namespaces.

In addition, `vtkm/cont/EncodePNG.h` and `vtkm/cont/DecodePNG.h` have been moved to a more natural home in `vtkm/io`.
