# VTK-m now supports dispatcher parameters being pointers

Previously it was only possible to pass values to a dispatcher when
you wanted to invoke a VTK-m worklet. This caused problems when it came
to designing new types that used inheritance as the types couldn't be
past as the base type to the dispatcher. To fix this issue we now
support invoking worklets with pointers as seen below.

```cpp
  vtkm::cont::ArrayHandle<T> input;
  //fill input

  vtkm::cont::ArrayHandle<T> output;
  vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;

  dispatcher(&input, output);
  dispatcher(input, &output); 
  dispatcher(&input, &output); 
```
