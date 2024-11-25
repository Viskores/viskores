## Simplify CellLocatorBase and PointLocatorBase

`CellLocatorBase` and `PointLocatorBase` used to use CRTP. However, this
pattern is unnecessary as the only method in the subclass they call is
`Build`, which does not need templating. The base class does not have to
call the `PrepareForExecution` method, so it can provide its own features
to derived classes more easily.

Also moved `CellLocatorBase` and `PointLocatorBase` out of the `internal`
namespace. Although they provide little benefit other than a base class, it
will make documenting its methods easier.
