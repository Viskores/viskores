# Add ability to get an array from a Field for a particular type

Previously, whenever you got an array from a `Field` object from a call to
an `ApplyPolicy`, you would get back a `VariantArrayHandle` that allows you
to cast to multiple types. To use that, you then have to cast it to
multiple different types and multiple different storage.

Often, this is what you want. If you are operating on a field, then you
want to cast to the native type. But there are also cases where you know a
specific type you want. For example, if you are operating on two fields, it
makes sense to find the exact type for the first field and then cast the
second field to that type if necessary rather than pointlessly unroll
templates for the cross of every possible combination. Also, we are not
unrolling for different storage types or attempting to create a virtual
array. Instead, we are using an `ArrayHandleMultiplexer` so that you only
have to compile for this array once.

This is done through a new version of `ApplyPolicy`. This version takes a
type of the array as its first template argument, which must be specified.
    
This requires having a list of potential storage to try. It will use that
to construct an `ArrayHandleMultiplexer` containing all potential types.
This list of storages comes from the policy. A `StorageList` item was added
to the policy.
    
Types are automatically converted. So if you ask for a `vtkm::Float64` and
field contains a `vtkm::Float32`, it will the array wrapped in an
`ArrayHandleCast` to give the expected type.

Here is an example where you are doing an operation on a field and
coordinate system. The superclass finds the correct type of the field. Your
result is just going to follow the type of the field.

``` cpp
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet CrossProduct::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  vtkm::cont::CoordinateSystem coords = inDataSet.GetCoordianteSystem();
  auto coordsArray = vtkm::filter::ApplyPolicy<T>(coords, policy);
```
