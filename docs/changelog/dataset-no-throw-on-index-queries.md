# DataSet queries for CellSet and Coordinate System Indices don't throw

Asking for the index of a `vtkm::cont::CellSet` or `vtkm::cont::CoordinateSystem` by
name now returns a `-1` when no matching item has been found instead of throwing
an exception.

This was done to make the interface of `vtkm::cont::DataSet` to follow the guideline
"Only unrepresentable things should raise exceptions". The index of a non-existent item
is representable by `-1` and therefore we shouldn't throw, like wise the methods that return
references can still throw exceptions as you can't have a reference to an non-existent item.

