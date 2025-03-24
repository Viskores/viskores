## Contour Polygons with Marching Cubes

Previously, the Marching Cubes contouring algorithm only had case tables for 3D
polyhedra. This means that if you attempted to contour or slice surfaces or
lines, you would not get any output. However, there are many valid use cases for
contouring this type of data.

This change adds case tables for triangles, quadrilaterals, and lines. It also
adds some special cases for general polygons and poly-lines. These special cases
do not use tables. Rather, there is a special routine that iterates over the
points since these cells types can have any number of points.

Note that to preserve the speed of the operation, contours for cells of a single
dimension type are done at one time. By default, the contour filter will try 3D
cells, then 2D cells, then 1D cells. It is also possible to select a particular
cell dimension or to append results from all cell types together. In this latest
case, the output cells will be of type `CellSetExplicit` instead of
`CellSetSingleType`.
