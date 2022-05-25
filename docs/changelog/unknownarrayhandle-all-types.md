# Do not require `VecTraits` for `UnknownArrayHandle` components

Whan an `UnknownArrayHandler` is constructed from an `ArrayHandle`, it uses
the `VecTraits` of the component type to construct its internal functions.
This meant that you could not put an `ArrayHandle` with a component type
that did not have `VecTraits` into an `UnknownArrayHandle`.

`UnknownArrayHandle` now no longer needs the components of its arrays to
have `VecTraits`. If the component type of the array does not have
`VecTraits`, it treats the components as if they are a scalar type.

