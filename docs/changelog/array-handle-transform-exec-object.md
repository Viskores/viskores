# Allow ArrayHandleTransform to work with ExecObject

Previously, the `ArrayHandleTransform` class only worked with plain old
data (POD) objects as is functors. For simple transforms, this makes sense
since all the data comes from a target `ArrayHandle` that will be sent to
the device through a different path. However, this also requires the
transform to be known at compile time.

However, there are cases where the functor cannot be a POD object and has
to be built for a specific device. There are numerous reasons for this. One
might be that you need some lookup tables. Another might be you want to
support a virtual object, which has to be initialized for a particular
device. The standard way to implement this in VTK-m is to create an
"executive object." This actually means that we create a wrapper around
executive objects that inherits from
`vtkm::cont::ExecutionAndControlObjectBase` that contains a
`PrepareForExecution` method and a `PrepareForControl` method.

As an example, consider the use case of a special `ArrayHandle` that takes
the value in one array and returns the index of that value in another
sorted array. We can do that by creating a functor that finds a value in an
array and returns the index.

``` cpp
template <typename ArrayPortalType>
struct FindValueFunctor
{
  ArrayPortalType SortedArrayPortal;
  
  FindValueFunctor() = default;
  
  VTKM_CONT FindValueFunctor(const ArrayPortalType& sortedPortal)
    : SortedArrayPortal(sortedPortal)
  { }
  
  VTKM_EXEC vtkm::Id operator()(const typename PortalType::ValueType& value)
  {
    vtkm::Id leftIndex = 0;
	vtkm::Id rightIndex = this->SortedArrayPortal.GetNubmerOfValues();
	while (leftIndex < rightIndex)
	{
	  vtkm::Id middleIndex = (leftIndex + rightIndex) / 2;
	  auto middleValue = this->SortedArrayPortal.Get(middleIndex);
	  if (middleValue <= value)
	  {
	    rightIndex = middleValue;
	  }
	  else
	  {
	    leftIndex = middleValue + 1;
	  }
	}
	return leftIndex;
  }
};
```

Simple enough, except that the type of `ArrayPortalType` depends on what
device the functor runs on (not to mention its memory might need to be
moved to different hardware). We can now solve this problem by creating a
functor objecgt set this up for a device. `ArrayHandle`s also need to be
able to provide portals that run in the control environment, and for that
we need a special version of the functor for the control environment.

``` cpp
template <typename ArrayHandleType>
struct FindValueExecutionObject : vtkm::cont::ExecutionAndControlObjectBase
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  
  ArrayHandleType SortedArray;
  
  FindValueExecutionObject() = default;
  
  VTKM_CONT FindValueExecutionObject(const ArrayHandleType& sortedArray)
    : SortedArray(sortedArray)
  { }
  
  template <typename Device>
  VTKM_CONT
  FindValueFunctor<decltype(std::declval<FunctorType>()(Device()))>
  PrepareForExecution(Device device)
  {
    using FunctorType =
	  FindValueFunctor<decltype(std::declval<FunctorType>()(Device()))>

    return FunctorType(this->SortedArray.PrepareForInput(device));
  }
  
  VTKM_CONT
  FundValueFunctor<typename ArrayHandleType::PortalConstControl>
  PrepareForControl()
  {
    using FunctorType =
	  FindValueFunctor<typename ArrayHandleType::PortalConstControl>
	
	return FunctorType(this->SortedArray.GetPortalConstControl());
  }
}
```

Now you can use this execution object in an `ArrayHandleTransform`. It will
automatically be detected as an execution object and be converted to a
functor in the execution environment.

``` cpp
auto transformArray = 
  vtkm::cont::make_ArrayHandleTransform(
    inputArray, FindValueExecutionObject<decltype(sortedArray)>(sortedArray));
```
