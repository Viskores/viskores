## Enable extracting external faces from curvilinear data

The external faces filter was not working with curvilinear data. The
implementation for structured cells was relying on axis-aligned point
coordinates, which is not the case for curvilinear grids. The implementation now
only relies on the indices in the 3D grid, so it works on structured data
regardless of the point coordinates. This should also speed up the operation.
