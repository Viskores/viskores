# Add test for array and datas that are cleaned up after finalize

It is the case that arrays might be deallocated from a device after the
device is closed. This can happen, for example, when an `ArrayHandle` is
declared globally. It gets constructed before VTK-m is initialized. This
is OK as long as you do not otherwise use it until VTK-m is initialized.
However, if you use that `ArrayHandle` to move data to a device and that
data is left on the device when the object closes, then the
`ArrayHandle` will be left holding a reference to invalid device memory
once the device is shut down. This can cause problems when the
`ArrayHandle` destructs itself and attempts to release this memory.

The VTK-m devices should gracefully handle deallocations that happen
after device shutdown.
