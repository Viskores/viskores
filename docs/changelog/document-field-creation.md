## Better document the creation of Field and CoordinateSystem

The constructors for `vtkm::cont::Field` and `vtkm::cont::CoordinateSystem`
were missing from the built user's guide. The construction of these classes
from names, associations, and arrays are now provided in the documentation.

Also added new versions of `AddField` and `AddCoordinateSystem` to
`vtkm::cont::DataSet` that mimic the constructors. This adds some sytatic
sugar so you can just emplace the field instead of constructing and
passing.
