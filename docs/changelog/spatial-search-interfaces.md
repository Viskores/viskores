#Interfaces for VTK-m spatial search strucutres added

The objective for this feature was to add a commom interface for the VTK-m
spatial search strucutes for ease of use for the users.
VTK-m now distinguishes locators into two types, cell locators and point 
locators. Cell locators can be used to query a containing cell for a point, 
and point locators can be used to search for other points that are close to the 
given point.

All cell locators are now required to inherit from the interface
`vtkm::cont::CellLocator`,  and all point locatos are required to inherit from 
the interface `vtkm::cont::PointLocator` 

These interfaces describe the necessary features that are required from either 
a cell locator, or a point locator and provided an easy way to use them in the
execution environment.

By deriving new search structures from these locator interfaces, it makes it
easier for users to build the underlying strucutres as well, abstracting away
complicated details. After providing all the required data from a 
`vtkm::cont::DataSet` object, the user only need to call the `Update` method 
on the object of `vtkm::cont::CellLocator`, or `vtkm::cont::PointLocator`.

For example, building the cell locator which used a Bounding Interval Hiererchy 
tree as a search structure, provided in the class 
`vtkm::cont::BoundingIntervalHierarchy` which inherits from 
`vtkm::cont::CellLocator`, only requires few steps.

```c++
  // Build a bounding interval hierarchy with 5 splitting planes,
  // and a maximum of 10 cells in the leaf node.
  vtkm::cont::BoundingIntervalHierarchy locator(5, 10);
  // Provide the cell set required by the search structure.
  locator.SetCellSet(cellSet);
  // Provide the coordinate system required by the search structure.
  locator.SetCoordinates(coords);
  // Cell the Update methods to finish building the underlying tree.
  locator.Update();
```
Similarly, users can easily build available point locators as well.

When using an object of `vtkm::cont::CellLocator`, or `vtkm::cont::PointLocator`
in the execution environment, they need to be passed to the worklet as an 
`ExecObject` argument. In the execution environment, users will receive a 
pointer to an object of type `vtkm::exec::CellLocator`, or 
`vtkm::exec::PointLocator` respectively. `vtkm::exec::CellLocator` provides a 
method `FindCell` to use in the execution environment to query the containing 
cell of a point. `vtkm::exec::PointLocator` provides a method 
`FindNearestNeighbor` to query for the nearest point.

As of now,  VTK-m provides only one implementation for each of the given 
interfaces. `vtkm::cont::BoundingIntervalHierarchy` which is an implementation 
of `vtkm::cont::CellLocator`, and `vtkm::cont::PointLocatorUniformGrid`, which 
is an implementation of `vtkm::cont::PointLocator`.
