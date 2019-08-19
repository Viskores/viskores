# Simplify creating Fields from ArrayHandles

VTK-m now offers `make_FieldPoint` and `make_FieldCell` functions
that reduce the complexity of construction `vtkm::cont::Fields`
from `vtkm::cont::ArrayHandles`.

Previously to construct a point and cell fields you would do:
```cpp
vtkm::cont::ArrayHandle<int> pointHandle;
vtkm::cont::ArrayHandle<int> cellHandle;
vtkm::cont::Field pointField("p", vtkm::cont::Field::Association::POINTS, pointHandle);
vtkm::cont::Field cellField("c", vtkm::cont::Field::Association::CELL_SET, "cells", cellHandle);
```

Now with the new `make_` functions you can do:
```cpp
vtkm::cont::ArrayHandle<int> pointHandle;
vtkm::cont::ArrayHandle<int> cellHandle;
auto pointField = vtkm::cont::make_FieldPoint("p", pointHandle);
auto cellField = vtkm::cont::make_FieldCell("c", "cells", cellHandle);
```
