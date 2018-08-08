# DeviceAdapterTags are usable for runtime device selection

VTK-m DeviceAdapterTags now are both a compile time representation of which device to use, and
also the runtime representation of that device. Previously the runtime representation was handled
by `vtkm::cont::DeviceAdapterId`. This was done by making `DeviceAdapterTag`'s' a constexpr type that
inherits from the constexpr `vtkm::cont::DeviceAdapterId` type.

At at ten thousand foot level this change means that in general instead of using `vtkm::cont::DeviceAdapterTraits<DeviceTag>`
you can simply use `DeviceTag`, or an instance of if `DeviceTag runtimeDeviceId;`.

Previously if you wanted to get the runtime representation of a device you would do the following:
```cpp
template<typename DeviceTag>
vtkm::cont::DeviceAdapterId getDeviceId()
{
  using Traits = vtkm::cont::DeviceAdapterTraits<DeviceTag>;
  return Traits::GetId();
}
...
vtkm::cont::DeviceAdapterId runtimeId = getDeviceId<DeviceTag>();
```
Now with the updates you could do the following.
```cpp
vtkm::cont::DeviceAdapterId runtimeId = DeviceTag();
```
More importantly this conversion is unnecessary as you can pass instances `DeviceAdapterTags` into methods or functions
that want `vtkm::cont::DeviceAdapterId` as they are that type!


Previously if you wanted to see if a DeviceAdapter was enabled you would the following:
```cpp
using Traits = vtkm::cont::DeviceAdapterTraits<DeviceTag>;
constexpr auto isValid = std::integral_constant<bool, Traits::Valid>();
```
Now you would do:
```cpp
constexpr auto isValid = std::integral_constant<bool, DeviceTag::IsEnabled>();
```

So why did VTK-m make these changes? 

That is a good question, and the answer for that is two fold. The VTK-m project is working better support for ArraysHandles that leverage runtime polymorphism (aka virtuals), and the ability to construct `vtkm::worklet::Dispatchers` without specifying 
the explicit device they should run on. Both of these designs push more of the VTK-m logic to operate at runtime rather than compile time. This changes are designed to allow for consistent object usage between runtime and compile time instead of having
to convert between compile time and runtime types.

