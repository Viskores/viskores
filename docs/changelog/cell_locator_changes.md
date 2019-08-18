# Updating structured cell locators

VTK-m will mow allow locating containing cells for a point using `CellLocatorUniformGrid`
and `CellLocatorRectilinearGrid` for 2D grids.

Users are required to create the locator objects as they normally would.
However, the `FindCell` method in `vtkm::exec::CellLocator` still requires users
to pass a 3D point as an input.

Further, the structured grid locators no longer use the `vtkm::exec::WorldToParametricCoordinates`
method to return parametric coordinates, instead they use fast paths for locating
points in a cell of an axis-aligned grid.

Another change for the `CellLocatorRectilinearGrid` is that now it uses binary search
on individual component arrays to search for a point.
