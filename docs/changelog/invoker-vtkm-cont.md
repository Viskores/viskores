# Invoker moved to vtkm::cont

Previously, `Invoker` was located in the `vtkm::worklet` namespace to convey
it was a replacement for using `vtkm::worklet::Dispatcher*`. In actuality
it should be in `vtkm::cont` as it is the proper way to launch worklets
for execution, and that shouldn't exist inside the `worklet` namespace.
