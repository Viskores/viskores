# Add initial support for aborting execution

VTK-m now has preliminary support for aborting execution. The per-thread instances of
`RuntimeDeviceTracker` have a functor called `AbortChecker`. This functor can be set using
`RuntimeDeviceTracker::SetAbortChecker()` and cleared by `RuntimeDeviceTracker::ClearAbortChecker()`
The abort checker functor should return `true` if an abort is requested for the thread,
otherwise, it should return `false`.

Before launching a new task, `TaskExecute` calls the functor to see if an abort is requested,
and If so, throws an exception of type `vtkm::cont::ErrorUserAbort`.

Any code that wants to use the abort feature, should set an appropriate `AbortChecker`
functor for the target thread. Then any piece of code that has parts that can execute on
the device should be put under a `try-catch` block. Any clean-up that is required for an
aborted execution should be handled in a `catch` block that handles exceptions of type
`vtkm::cont::ErrorUserAbort`.

The limitation of this implementation is that it is control-side only. The check for abort
is done before launching a new device task. Once execution has begun on the device, there is
currently no way to abort that. Therefore, this feature is only useful for aborting code
that is made up of several smaller device task launches (Which is the case for most
worklets and filters in VTK-m)
