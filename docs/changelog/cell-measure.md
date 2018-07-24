# Cell measure functions, worklet, and filter

VTK-m now provides free functions, a worklet, and a filter for computing
the integral measure of a cell (i.e., its arc length, area, or volume).

The free functions are located in `vtkm/exec/CellMeasure.h` and share the
same signature:

```c++
  template<typename OutType, typename PointVecType>
  OutType CellMeasure(
    const vtkm::IdComponent& numPts,
    const PointCoordVecType& pts,
    CellShapeTag,
    const vtkm::exec::FunctorBase& worklet);
```

The number of points argument is provided for cell-types such as lines,
which allow an arbitrary number of points per cell.
See the worklet for examples of their use.

The worklet is named `vtkm::worklet::CellMeasure` and takes a template
parameter that is a tag list of measures to include.
Cells that are not selected by the tag list return a measure of 0.
Some convenient tag lists are predefined for you:

+ `vtkm::ArcLength` will only compute the measure of cells with a 1-dimensional parameter-space.
+ `vtkm::Area` will only compute the measure of cells with a 2-dimensional parameter-space.
+ `vtkm::Volume` will only compute the measure of cells with a 3-dimensional parameter-space.
+ `vtkm::AllMeasures` will compute all of the above.

The filter version, named `vtkm::filter::CellMeasures` – plural since
it produces a cell-centered array of measures — takes the same template
parameter and tag lists as the worklet.
By default, the output array of measure values is named "measure" but
the filter accepts other names via the `SetCellMeasureName()` method.

The only cell type that is not supported is the polygon;
you must triangulate polygons before running this filter.
See the unit tests for examples of how to use the worklet and filter.

The cell measures are all signed: negative measures indicate that the cell is inverted.
Simplicial cells (points, lines, triangles, tetrahedra) cannot not be inverted
by definition and thus always return values above or equal to 0.0.
Negative values indicate either the order in which vertices appear in its connectivity
array is improper or the relative locations of the vertices in world coordinates
result in a cell with a negative Jacobian somewhere in its interior.
Finally, note that cell measures may return invalid (NaN) or infinite (Inf, -Inf)
values if the cell is poorly defined, e.g., has coincident vertices
or a parametric dimension larger than the space spanned by its world-coordinate
vertices.

The verdict mesh quality library was used as the source of the methods
for approximating the cell measures.
