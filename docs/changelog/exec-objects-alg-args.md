# Support ExecArg behavior in `vtkm::cont::Algorithm` methods

`vtkm::cont::Algorithm` is a wrapper around `DeviceAdapterAlgorithm` that
internally uses `TryExecute`s to select an appropriate device. The
intention is that you can run parallel algorithms (outside of worklets)
without having to specify a particular device.

Most of the arguments given to device adapter algorithms are actually
control-side arguments that get converted to execution objects internally
(usually a `vtkm::cont::ArrayHandle`). However, some of the algorithms,
take an argument that is passed directly to the execution environment, such
as the predicate argument of `Sort`. If the argument is a plain-old-data
(POD) type, which is common enough, then you can just pass the object
straight through. However, if the object has any special elements that have
to be transferred to the execution environment, such as internal arrays,
passing this to the `vtkm::cont::Algorithm` functions becomes problematic.

To cover this use case, all the `vtkm::cont::Algorithm` functions now
support automatically transferring objects that support the `ExecObject`
worklet convention. If any argument to any of the `vtkm::cont::Algorithm`
functions inherits from `vtkm::cont::ExecutionObjectBase`, then the
`PrepareForExecution` method is called with the device the algorithm is
running on, which allows these device-specific objects to be used without
the hassle of creating a `TryExecute`.
