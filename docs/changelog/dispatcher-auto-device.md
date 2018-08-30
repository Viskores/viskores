# Make DispatcherBase invoke using a TryExecute

Rather than force all dispatchers to be templated on a device adapter,
instead use a TryExecute internally within the invoke to select a device
adapter.
    
Because this removes the need to declare a device when invoking a worklet,
this commit also removes the need to declare a device in several other
areas of the code.

This changes touches quite a bit a code. The first pass of the change
usually does the minimum amount of work, which is to change the
compile-time specification of the device to a run-time call to `SetDevice`
on the dispatcher. Although functionally equivalent, it might mean calling
`TryExecute` within itself.
