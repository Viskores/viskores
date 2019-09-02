# vtkm::cont::Filter now don't have an active cell set

`vtkm::filter::FilterField` has removed the concept of `ActiveCellSetIndex`. This
has been done as `vtkm::cont::DataSet` now only contains a single `vtkm::cont::CellSet`.
