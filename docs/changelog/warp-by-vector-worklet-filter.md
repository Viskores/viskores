# Add a warp vector worklet and filter

This commit adds a worklet that modifies point coordinates by moving points
along point normals by the scalar amount. It's a simpified version of the
vtkWarpScalar in VTK. Additionally the filter doesn't modify the point coordinates,
but creates a new point coordinates that have been warped.
Useful for showing flow profiles or mechanical deformation.
