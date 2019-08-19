# Simplify creating results for Filters

As part of the process of making VTK-m filters easier to write for newcomers
whe have a couple of changes to make constructing the output `vtkm::cont::DataSet`
easier.

First we have moved the `CreateResult` functions out of the internals namespace
and directly into `vtkm::filter`. This makes it clearer to developers that this
was the 'proper' way to construct the output DataSet.

Second we have streamlined the collection of `vtkm::filter::CreateResult` methods to
require the user to provide less information and provide clearer names explaing what
they do.

To construct output identical to the input but with a new field you now just pass
the `vtkm::filter::FieldMetadata` as a paramter instead of explictly stating
the field association, and the possible cell set name:
```cpp
return CreateResult(input, newField, name, fieldMetadata);
```

To construct output identical to the input but with a cell field added you
can now pass the `vtkm::cont::CellSet` as a paramter instead of explictly stating
the field association, and the cell set name:
```cpp
return CreateResultFieldCell(input, newCellField, name, cellset);
```
