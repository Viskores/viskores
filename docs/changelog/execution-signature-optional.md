ExecutionSignatures are now optional for simple worklets

If a worklet doesn't explicitly state an `ExecutionSignature`, VTK-m
assumes the worklet has no return value, and each `ControlSignature`
argument is passed to the worklet in the same order.

For example if we had this worklet:
```cxx
struct DotProduct : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename T, vtkm::IdComponent Size>
  VTKM_EXEC void operator()(const vtkm::Vec<T, Size>& v1,
                            const vtkm::Vec<T, Size>& v2,
                            T& outValue) const
  {
    outValue = vtkm::Dot(v1, v2);
  }
};
```

It can be simplified to be:

```cxx
struct DotProduct : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T, vtkm::IdComponent Size>
  VTKM_EXEC void operator()(const vtkm::Vec<T, Size>& v1,
                            const vtkm::Vec<T, Size>& v2,
                            T& outValue) const
  {
    outValue = vtkm::Dot(v1, v2);
  }
};
