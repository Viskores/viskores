# Fields now don't require the associated CellSet name

Now that `vtkm::cont::DataSet` can only have a single `vtkm::cont::CellSet`
the requirement that cell based `vtkm::cont::Field`s need a CellSet name
has been lifted.
