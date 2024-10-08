## Simplify CellLocatorBase

`CellLocatorBase` used to use CRTP. However, this pattern is unnecessary as
the only real subclass it calls is `Build`, which does not need templating.
The base class does not have to call the `PrepareForExecution` method, so
it can provide its own features to derived classes more easily.

Also moved `CellLocatorBase` out of the `internal` namespace. Although it
provides little benefit other than a base class, it will make documenting
its methods easier.
