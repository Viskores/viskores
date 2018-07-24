Add a warpScalar worklet and filter

This commit adds a worklet as well as a filter that modify point coordinates by moving points
along point normals by the scalar amount times the scalar factor.
It's a simpified version of the vtkWarpScalar class in VTK. Additionally the filter doesn't
modify the point coordinates, but creates a new point coordinates that have been warped.

