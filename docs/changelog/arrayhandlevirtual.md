# Add vtkm::cont::ArrayHandleVirtual 


Added a new class named `ArrayHandleVirtual` that allows you to type erase an
ArrayHandle storage type by using virtual calls. This simplification makes
storing `Fields` and `Coordinates` significantly easier as VTK-m doesn't
need to deduce both the storage and value type when executing worklets.

To construct an `ArrayHandleVirtual` one can do one of the following:

```cpp
vtkm::cont::ArrayHandle<vtkm::Float32> pressure;
vtkm::cont::ArrayHandleConstant<vtkm::Float32> constant(42.0f);


// constrcut from an array handle
vtkm::cont::ArrayHandleVirtual<vtkm::Float32> v(pressure);

// or assign from an array handle
v = constant;

```

To help maintain performance `ArrayHandleVirtual` provides a collection of helper
functions/methods to query and cast back to the concrete storage and value type:
```cpp
vtkm::cont::ArrayHandleConstant<vtkm::Float32> constant(42.0f);
vtkm::cont::ArrayHandleVirtual<vtkm::Float32> v = constant;

bool isConstant = vtkm::cont::IsType< decltype(constant) >(v);
if(isConstant)
  vtkm::cont::ArrayHandleConstant<vtkm::Float32> t = vtkm::cont::Cast< decltype(constant) >(v);

```

Lastly, a common operation of calling code using `ArrayHandleVirtual` is a desire to construct a new instance
of an existing virtual handle with the same storage type. This can be done by using the `NewInstance` method
as seen below
```cpp
vtkm::cont::ArrayHandle<vtkm::Float32> pressure;
vtkm::cont::ArrayHandleVirtual<vtkm::Float32> v = pressure;

vtkm::cont::ArrayHandleVirtual<vtkm::Float32> newArray = v->NewInstance();
bool isConstant = vtkm::cont::IsType< vtkm::cont::ArrayHandle<vtkm::Float32> >(newArray); //will be true
```
