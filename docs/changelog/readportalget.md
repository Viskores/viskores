# `ReadPortal().Get(idx)`

Calling `ReadPortal()` in a tight loop is an antipattern.
A call to `ReadPortal()` causes the array to be copied back to the control environment,
and hence code like

```cpp
for (vtkm::Id i = 0; i < array.GetNumberOfValues(); ++i) {
    vtkm::FloatDefault x = array.ReadPortal().Get(i);
}
```

is a quadratic-scaling loop.

We have remove *almost* all internal uses of the `ReadPortal().Get` antipattern,
with the exception of 4 API calls into which the pattern is baked in:
`CellSetExplicit::GetCellShape`, `CellSetPermutation::GetNumberOfPointsInCell`, `CellSetPermutation::GetCellShape`, and `CellSetPermutation::GetCellPointIds`.
We expect these will need to be deprecated in the future.
