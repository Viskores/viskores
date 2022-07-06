# Fix bug with voxels in legacy vtk files

The legacy VTK file reader for unstructured grids had a bug when reading
cells of type voxel. VTK-m does not support the voxel cell type in
unstructured grids (i.e. explicit cell sets), so it has to convert them to
hexahedron cells. A bug in the reader was mangling the cell array index
during this conversion.
