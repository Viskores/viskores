# Add new execution and control objects

[Recent changes to execution objects](change-execution-object-creation.md)
now have execution objects behave as factories that create an object
specific for a particular device. Sometimes, you also need to be able to
get an object that behaves properly in the control environment. For these
cases, a sublcass to `vtkm::cont::ExecutionObjectBase` was created.

This subclass is called `vtkm::cont::ExecutionAndControlObjectBase`. In
addition to the `PrepareForExecution` method required by its superclass,
these objects also need to provide a `PrepareForControl` method to get an
equivalent object that works in the control environment.

See [the changelog for execution objects in
`ArrayHandleTransform`](array-handle-transform-exec-object.md) for an
example of using a `vtkm::cont::ExecutionAndControlObjectBase`.
