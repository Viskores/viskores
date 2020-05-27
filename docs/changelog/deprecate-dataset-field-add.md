# Deprecate `DataSetFieldAdd`

The class `vtkm::cont::DataSetFieldAdd` is now deprecated.
Its methods, `AddPointField` and `AddCellField` have been moved to member functions
of `vtkm::cont::DataSet`, which simplifies many calls.

For example, the following code

```cpp
vtkm::cont::DataSetFieldAdd fieldAdder;
fieldAdder.AddCellField(dataSet, "cellvar", values);
```

would now be

```cpp
dataSet.AddCellField("cellvar", values);
```
