# Use the strong typed enums for vtkm::cont::Field

By doing so, the compiler would not convert these enums into `int`s
which can cause some unexpected behavior.
