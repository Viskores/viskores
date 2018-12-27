# vtkm::cont::VariantArrayHandle replaces vtkm::cont::DynamicArrayHandle

`ArrayHandleVariant` replaces `DynamicArrayHandle` as the primary method
for holding onto a type erased `vtkm::cont::ArrayHandle`. The major difference
between the two implementations is how they handle the Storage component of
an array handle.

`DynamicArrayHandle` approach was to find the fully deduced type of the `ArrayHandle`
meaning it would check all value and storage types it knew about until it found a match.
This cross product of values and storages would cause significant compilation times when
a `DynamicArrayHandle` had multiple storage types.

`VariantArrayHandle` approach is to only deduce the value type of the `ArrayHandle` and
return a `vtkm::cont::ArrayHandleVirtual` which uses polymorpishm to hide the actual
storage type. This approach allows for better compile times, and for calling code
to always expect an `ArrayHandleVirtual` instead of the fully deduced type. This conversion
to `ArrayHandleVirtual` is usually done internally within VTK-m when a  worklet or filter
is invoked.

In certain cases users of `VariantArrayHandle` want to be able to access the concrete 
`ArrayHandle<T,S>` and not have it wrapped in a `ArrayHandleVirtual`. For those occurrences
`VariantArrayHandle` provides a collection of helper functions/methods to query and
cast back to the concrete storage and value type:
```cpp
vtkm::cont::ArrayHandleConstant<vtkm::Float32> constant(42.0f);
vtkm::cont::ArrayHandleVariant v(constant);

bool isConstant = vtkm::cont::IsType< decltype(constant) >(v);
if(isConstant)
  vtkm::cont::ArrayHandleConstant<vtkm::Float32> t = vtkm::cont::Cast< decltype(constant) >(v);

```

Lastly, a common operation of calling code using `VariantArrayHandle` is a desire to construct a new instance
of an existing virtual handle with the same storage type. This can be done by using the `NewInstance` method
as seen below
```cpp
vtkm::cont::ArrayHandle<vtkm::Float32> pressure;
vtkm::cont::ArrayHandleVariant v(pressure);

vtkm::cont::ArrayHandleVariant newArray = v->NewInstance();
bool isConstant = vtkm::cont::IsType< decltype(pressure) >(newArray); //will be true
```
