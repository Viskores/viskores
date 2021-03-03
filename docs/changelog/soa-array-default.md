Support `ArrayHandleSOA` as a "default" array

Many programs, particularly simulations, store fields of vectors in
separate arrays for each component. This maps to the storage of
`ArrayHandleSOA`. The VTK-m code tends to prefer the AOS storage (which is
what is implemented in `ArrayHandleBasic`, and the behavior of which is
inherited from VTK). VTK-m should better support adding `ArrayHandleSOA` as
one of the types.

We now have a set of default types for Ascent that uses SOA as one of the
basic types.

Part of this change includes an intentional feature regression of
`ArrayHandleSOA` to only support value types of `Vec`. Previously, scalar
types were supported. However, the behavior of `ArrayHandleSOA` is exactly
the same as `ArrayHandleBasic`, except a lot more template code has to be
generated. That itself is not a huge deal, but because you have 2 types
that essentially do the same thing, a lot of template code in VTK-m would
unwind to create two separate code paths that do the same thing with the
same data. To avoid creating those code paths, we simply make any use of
`ArrayHandleSOA` without a `Vec` value invalid. This will prevent VTK-m
from creating those code paths.
