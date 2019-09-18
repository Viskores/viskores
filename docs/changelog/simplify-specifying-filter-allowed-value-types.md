# Provide a simplified way to state allowed value types for VTK-m filters

Previously VTK-m filters used a specialization of `vtkm::filter::FilterTraits<>` to control
the acceptable input value types. For example if the `WarpVector` filter want to only allow
`vtkm::Vec3f_32` and `vtkm::Vec3f_64` it would use:

```cpp
namespace vtkm { namespace filter {
template <>
class FilterTraits<WarpVector>
{
public:
  // WarpVector can only applies to Float and Double Vec3 arrays
  using InputFieldTypeList = vtkm::TypeListTagFieldVec3;
};
}}
```

This increase the complexity of writing filters. To make this easier VTK-m now looks for
a `SupportedTypes` define on the filter when a `vtkm::filter::FilterTraits` specialization
doesn't exist. This allows filters to succinctly specify supported types, such as seen below
for the `WarpVector` filter. 

```cpp
class WarpVector : public vtkm::filter::FilterField<WarpVector>
{
public:
  using SupportedTypes = vtkm::TypeListTagFieldVec3;
...
};
```
