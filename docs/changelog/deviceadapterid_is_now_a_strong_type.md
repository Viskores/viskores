#DeviceAdapterId has becomes a real constexpr type and not an alias to vtkm::UInt8

As part of the ability to support `vtkm::cont::TryExecuteOnDevice` VTK-m has made the
DeviceAdapterId a real constexpr type instead of a vtkm::UInt8.

The benefits of a real type are as follows:

- Easier to add functionality like range verification, which previously had
  to be located in each user of `DeviceAdapterId`

- In ability to have ambiguous arguments. Previously it wasn't perfectly clear
  what a method parameter of `vtkm::UInt8` represented. Was it actually the
  DeviceAdapterId or something else?

- Ability to add subclasses that represent things such as Undefined, Error, or Any.


The implementation of DeviceAdapterId is:
```cpp
struct DeviceAdapterId
{
  constexpr explicit DeviceAdapterId(vtkm::Int8 id)
    : Value(id)
  {
  }

  constexpr bool operator==(DeviceAdapterId other) const { return this->Value == other.Value; }
  constexpr bool operator!=(DeviceAdapterId other) const { return this->Value != other.Value; }
  constexpr bool operator<(DeviceAdapterId other) const { return this->Value < other.Value; }

  constexpr bool IsValueValid() const
  {
    return this->Value > 0 && this->Value < VTKM_MAX_DEVICE_ADAPTER_ID;
  }

  constexpr vtkm::Int8 GetValue() const { return this->Value; }

private:
  vtkm::Int8 Value;
};
```
