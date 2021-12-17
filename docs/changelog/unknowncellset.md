# UnknownCellSet

The `DynamicCellSet` class has been replaced with `UnknownCellSet`.
Likewise, the `DynamicCellSetBase` class (a templated version of
`DynamicCellSet`) has been replaced with `UncertainCellSet`.

These changes principally follow the changes to the `UnknownArrayHandle`
management class. The `ArrayHandle` version of a polymorphic manager has
gone through several refinements from `DynamicArrayHandle` to
`VariantArrayHandle` to its current form as `UnknownArrayHandle`.
Throughout these improvements for `ArrayHandle`, the equivalent classes for
`CellSet` have lagged behind. The `CellSet` version is decidedly simpler
because `CellSet` itself is polymorphic, but there were definitely
improvements to be had.

The biggest improvement was to remove the templating from the basic unknown
cell set. The old `DynamicArrayHandle` was actually a type alias for
`DynamicArrayHandleBase<VTKM_DEFAULT_CELL_SET_LIST>`. As
`VTKM_DEFAULT_CELL_SET_LIST` tends to be pretty long, `DynamicArrayHandle`
was actually a really long type. In contrast, `UnknownArrayHandle` is its
own untemplated class and will show up in linker symbols as such.
