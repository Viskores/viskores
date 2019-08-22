# FilterField now provides all functionality of FilterCell

The FilterCell was a subclass of `vtkm::filter::FilterField` and behaves essentially the same
but provided the pair of methods `SetActiveCellSetIndex` and `GetActiveCellSetIndex`.
It was a common misconception that `FilterCell` was meant for Cell based algorithms, instead of
algorithms that required access to the active `vtkm::cont::CellSet`.

By moving `SetActiveCellSetIndex` and `GetActiveCellSetIndex` to FilterField, we remove this confusion.

