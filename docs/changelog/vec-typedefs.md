# Add aliases for common Vec types

Specifying `Vec` types can be verbose. For example, to simply express a
vector in 3-space, you would need a declaration like this:

``` cpp
vtkm::Vec<vtkm::FloatDefault, 3>
```

This is very verbose and frankly confusing to users. To make things easier,
we have introduced several aliases for common `Vec` types. For example, the
above type can now be referenced simply with `vtkm::Vec3f`, which is a
3-vec of floating point values of the default width. If you want to specify
the width, then you can use either `vtkm::Vec3f_32` or `vtkm::Vec3f_64`.

There are likewise types introduced for integers and unsigned integers
(e.g. `vtkm::Vec3i` and `vtkm::Vec3ui`). You can specify the width of these
all the way down to 8 bit (e.g. `vtkm::Vec3ui_8`, `vtkm::Vec3ui_16`,
`vtkm::Vec3ui_32`, and `vtkm::Vec3ui_64`).

For completeness, `vtkm::Id4` was added as well as `vtkm::IdComponent2`,
`vtkm::IdComponent3`, and `vtkm::IdComponent4`.
