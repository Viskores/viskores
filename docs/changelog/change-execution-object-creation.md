# User defined execution objects now usable with runtime selection of device adapter

- Changed how Execution objects are created and passed from the cont environment to the execution environment. See chapter 13.9 on worklets in the user manual for details.

- Instead we will now fill out a class and call prepareForExecution() and create the execution object for the execution environment from this function. This way we do not have to template the class that extends `vtkm::cont::ExecutionObjectBase` on the device.

Example of new execution object:
```cpp
template <typename Device>
struct ExecutionObject
{
  vtkm::Int32 Number;
};

struct TestExecutionObject : public vtkm::cont::ExecutionObjectBase
{
  vtkm::Int32 Number;

  template <typename Device>
  VTKM_CONT ExecutionObject<Device> PrepareForExecution(Device) const
  {
    ExecutionObject<Device> object;
    object.Number = this->Number;
    return object;
  }
};
```
