# Enable `TypeToString` for `type_info`

VTK-m contains a helpful method named `vtkm::cont::TypeToString` that
either takes a type as a template argument or a `std::type_info` object and
returns a human-readable string for that type.

The standard C++ library has an alternate for `std::type_info` named
`std::type_index`, which has the added ability to be used in a container
like `set` or `map`. The `TypeToString` overloads have been extended to
also accept a `std::type_info` and report the name of the type stored in it
(rather than the name of `type_info` itself).
