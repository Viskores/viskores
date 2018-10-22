# Remove TryExecute from filter

[A recent change to dispatchers](dispatcher-auto-device.md) has embedded a
`TryExecute` internally within the `Invoke` function of all dispatchers. This
means that it is no longer necessary to specify a device when invoking a 
worklet.

Previously, this `TryExecute` was in the filter layer. The filter superclasses
would do a `TryExecute` and use that to pass to subclasses in methods like
`DoExecute` and `DoMapField`. Since the dispatcher no longer needs a device
this `TryExecute` is generally unnecessary and always redundant. Thus, it has
been removed.

Because of this, the device argument to `DoExecute` and `DoMapField` has been
removed. This will cause current implementations of filter to change, but it
usually simplifies code. That said, there might be some code that needs to be
wrapped into a `vtkm::cont::ExecObjectBase`.

No changes need to be made to code that uses filters.
