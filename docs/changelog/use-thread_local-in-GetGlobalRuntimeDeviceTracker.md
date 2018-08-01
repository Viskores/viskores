# Use thread_local in GetGlobalRuntimeDeviceTracker function if possible

It will reduce the cost of getting the thread runtime device tracker,
and will have a better runtime overhead if user constructs a lot of
short lived threads that use VTK-m.
