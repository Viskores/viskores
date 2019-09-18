# Invoker now a member of all vtkm::filters

To simplify how vtkm filters are written we have made each vtkm::filter
have a `vtkm::cont::Invoker` as member variable. The goal with this change
is provide an uniform API for launching all worklets from within a filter.

Lets consider the PointElevation filter. Previous to these changes the
`DoExecute` would need to construct the correct dispatcher with the
correct parameters as seen below:

```cpp
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet PointElevation::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;

  vtkm::worklet::DispatcherMapField<vtkm::worklet::PointElevation> dispatcher(this->Worklet);
  dispatcher.Invoke(field, outArray);
  ...
}
```

With these changes the filter can instead use `this->Invoke` and have
the correct dispatcher executed. This makes it easier to teach and
learn how to write new filters.
```cpp
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet PointElevation::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;

  this->Invoke(this->Worklet, field, outArray);
  ...
}
```
