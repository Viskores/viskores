# Provide pre-built filters in the vtkm_filter library.

VTK-m now provides the following pre built versions of
the following filters as part of the vtkm_filter library,
when executed with the default types.
  - CellAverage
  - CleanGrid
  - ClipWithField
  - ClipWithImplicitFunction
  - Contour
  - ExternalFaces
  - ExtractStuctured
  - PointAverage
  - Threshold
  - VectorMagnitude

The decision on providing a subset of filters as a library
was based on balancing the resulting library size and cross domain
applicibaility of the filter. So the initial set of algorithms
have been selected by looking at what is commonly used by
current VTK-m consuming applications.

By default types we mean that no explicit user policy has been
passed to the `Execute` method on these filters. For example
the following will use the pre-build `Threshold` and `CleanGrid`
filters:

```cpp
  vtkm::cont::DataSet input = ...;

  //convert input to an unstructured grid
  vtkm::filter::CleanGrid clean;
  auto cleaned = clean.Execute(input);

  vtkm::filter::Threshold threshold;
  threshold.SetLowerThreshold(60.1);
  threshold.SetUpperThreshold(60.1);
  threshold.SetActiveField("pointvar");
  threshold.SetFieldsToPass("cellvar");
  auto output = threshold.Execute(cleaned);
  ...
```

While the following, even though it is a subset of the default
policy will need to be compiled by the consuming library by
including the relevant `.hxx` files

```cpp
  #include <vtkm/filter/CleanGrid.hxx>
  #include <vtkm/filter/Threshold.hxx>

  ...
  struct CustomPolicy : vtkm::filter::PolicyBase<CustomPolicy>
  {
    // Defaults are the same as PolicyDefault expect for the field types
    using FieldTypeList = vtkm::ListTagBase<vtkm::FloatDefault, vtkm::Vec3f>;
  };
  ...

  vtkm::cont::DataSet input = ...;

  //convert input to an unstructured grid
  vtkm::filter::CleanGrid clean;
  auto cleaned = clean.Execute(input, CustomPolicy{});

  vtkm::filter::Threshold threshold;
  threshold.SetLowerThreshold(60.1);
  threshold.SetUpperThreshold(60.1);
  threshold.SetActiveField("pointvar");
  threshold.SetFieldsToPass("cellvar");
  auto output = threshold.Execute(cleaned, CustomPolicy{});
  ...
```
