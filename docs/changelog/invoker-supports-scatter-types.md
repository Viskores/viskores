# `vtkm::worklet::Invoker` now able to worklets that have non-default scatter type

This change allows the `Invoker` class to support launching worklets that require
a custom scatter operation. This is done by providing the scatter as the second
argument when launch a worklet with the `()` operator.

The following example shows a scatter being provided with a worklet launch.

```cpp
struct CheckTopology : vtkm::worklet::WorkletMapPointToCell
{
  using ControlSignature = void(CellSetIn cellset, FieldOutCell);
  using ExecutionSignature = _2(FromIndices);
  using ScatterType = vtkm::worklet::ScatterPermutation<>;
  ...
};


vtkm::worklet::Ivoker invoke;
invoke( CheckTopology{}, vtkm::worklet::ScatterPermutation{}, cellset, result );
```
