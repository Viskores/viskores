# Add hints to device adapter scheduler

The `DeviceAdapter` provides an abstract interface to the accelerator
devices worklets and other algorithms run on. As such, the programmer has
less control about how the device launches each worklet. Each device
adapter has its own configuration parameters and other ways to attempt to
optimize how things are run, but these are always a universal set of
options that are applied to everything run on the device. There is no way
to specify launch parameters for a particular worklet.

To provide this information, VTK-m now supports `Hint`s to the device
adapter. The `DeviceAdapterAlgorithm::Schedule` method takes a templated
argument that is of the type `HintList`. This object contains a template
list of `Hint` types that provide suggestions on how to launch the parallel
execution. The device adapter will pick out hints that pertain to it and
adjust its launching accordingly.

These are called hints rather than, say, directives, because they don't
force the device adapter to do anything. The device adapter is free to
ignore any (and all) hints. The point is that the device adapter can take
into account the information to try to optimize for itself.

A provided hint can be tied to specific device adapters. In this way, an
worklet can further optimize itself. If multiple hints match a device
adapter, the last one in the list will be selected.

The `Worklet` base now has an internal type named `Hints` that points to a
`HintList` that is applied when the worklet is scheduled. Derived worklet
classes can provide hints by simply defining their own `Hints` type.

This feature is experimental and consequently hidden in an `internal`
namespace.
