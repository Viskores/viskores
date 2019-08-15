# FilterField now tries to be smart when selecting active cellset

Now when a calls a `vtkm::filter::FilterField` algorithm without an explicit
active CellSet, if the field is cell based we set the active `vtkm::cont::CellSet`
to be the one associated with that field. If that `vtkm::cont::CellSet` doesn't
exist we default back to using the first CellSet in the input `vtkm::cont::DataSet`.
