# Consolidate WarpScalar and WarpVector filter

In reflection, the `WarpScalar` filter is surprisingly a superset of the
`WarpVector` features. `WarpScalar` has the ability to displace in the
directions of the mesh normals. In VTK, there is a distinction of normals
to vectors, but in VTK-m it is a matter of selecting the correct one. As
such, it makes little sense to have two separate implementations for the
same operation. The filters have been combined and the interface names have
been generalized for general warping (e.g., "normal" or "vector" becomes
"direction").

In addition to consolidating the implementation, the `Warp` filter
implementation has been updated to use the modern features of VTK-m's
filter base classes. In particular, when the `Warp` filters were originally
implemented, the filter base classes did not support more than one active
scalar field, so filters like `Warp` had to manage multiple fields
themselves. The `FilterField` base class now allows specifying multiple,
indexed active fields, and the updated implementation uses this to manage
the input vectors and scalars.

The `Warp` filters have also been updated to directly support constant
vectors and scalars, which is common for `WarpScalar` and `WarpVector`,
respectively. Previously, to implement a constant field, you had to add a
field containing an `ArrayHandleConstant`. This is still supported, but an
easier method of just selecting constant vectors or scalars makes this
easier.

Internally, the implementation now uses tricks with extracting array
components to support many different array types (including
`ArrayHandleConstant`. This allows it to simultaneously interact with
coordinates, directions, and scalars without creating too many template
instances.
