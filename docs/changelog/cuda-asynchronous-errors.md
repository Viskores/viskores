# Worklets are now asynchronous in Cuda

Worklets are now fully asynchronous in the cuda backend. This means that
worklet errors are reported asynchonously. Existing errors are checked for
before invocation of a new worklet and at explicit synchronization points like
`DeviceAdapterAlgorithm<>::Synchronize()`.

An important effect of this change is that functions that are synchronization
points, like `ArrayHandle::GetPortalControl()` and
`ArrayHandle::GetPortalConstControl()`, may now throw exception for errors from
previously executed worklets.

Worklet invocations, synchronization and error reporting happen independtly
on different threads. Therefore, synchronization on one thread does not affect
any other threads.
