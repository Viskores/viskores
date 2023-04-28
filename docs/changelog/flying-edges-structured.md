# Implement Flying Edges for structured cellsets with rectilinear and curvilinear coordinates

When Flying Edges was introduced to compute contours of a 3D structured cellset, it could only process uniform coordinates. This limitation is now lifted : an alternative interpolation function can be used in the fourth pass of the algorithm in order to support rectilinear and curvilinear coordinate systems.

Accordingly, the `Contour` filter now calls `ContourFlyingEdges` instead of `ContourMarchingCells` for these newly supported cases.
