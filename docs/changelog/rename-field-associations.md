# Rename field associations

The symbols in `vtkm::cont::Field::Association` have been changed from
`ANY`, `WHOLE_MESH`, `POINTS`, and `CELL_SET` to `Any`, `WholeMesh`,
`Points`, and `Cells`, respectively. The reason for this change is twofold:

  * The general standard that VTK-m follows for `enum struct` enumerators
    is to use camel case (with the first character capitalized), not all
    upper case.
  * The use of `CELL_SET` for fields associated with cells is obsolete. A
    `DataSet` used to support having more than one `CellSet`, and so a
    field association on cells was actually bound to a particular
    `CellSet`. However, that is no longer the case. A `DataSet` has exactly
    one `CellSet`, so a cell field no longer has to point to a `CellSet`.
    Thus the enumeration symbol for `Cells` should match the one for
    `Points`.

For backward compatibility, the old enumerations still exist. They are
aliases for the new names, and they are marked as deprecated, so using them
will result in a compiler warning (on some systems).
