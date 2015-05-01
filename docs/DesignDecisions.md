### Design Decisions ###

A quick list of what the final design of vtkm should look like:

Code Layout:
```
vtkm/
  cont/
    - vtkm::cont::ArrayHandle
    - vtkm::cont::CellSet
    - vtkm::cont::DataSet

    interop/
      - OpenGL interop classes
      - VTK interop classes
    cuda/

  filters/
    - vtkm::filter::ThresholdFilter
    - vtkm::filter::ContourFilter
    - Mutators?
  exec/
    cuda/

  worklets/
    - vtkm::worklet::WorkletMapField
    - vtkm::worklet::WorkletMapCell
```



