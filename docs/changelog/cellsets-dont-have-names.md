# CellSets now don't have a name

The requirement that `vtkm::cont::CellSets` have a name was so
cell based `vtkm::cont::Field`'s could be associated with the
correct CellSet in a `vtkm::cont::DataSet`. 

Now that `DataSet`'s don't support multiple CellSets, we can remove
the `CellSet` name member variable.
