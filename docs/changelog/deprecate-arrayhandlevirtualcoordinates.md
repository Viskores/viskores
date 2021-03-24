# Deprecate ArrayHandleVirtualCoordinates

As we port VTK-m to more types of accelerator architectures, supporting
virtual methods is becoming more problematic. Thus, we are working to back
out of using virtual methods in the execution environment.

One of the most widespread users of virtual methods in the execution
environment is `ArrayHandleVirtual`. As a first step of deprecating this
class, we first deprecate the `ArrayHandleVirtualCoordinates` subclass.

Not surprisingly, `ArrayHandleVirtualCoordinates` is used directly by
`CoordinateSystem`. The biggest change necessary was that the `GetData`
method returned an `ArrayHandleVirtualCoordinates`, which obviously would
not work if that class is deprecated.

An oddness about this return type is that it is quite different from the
superclass's method of the same name. Rather, `Field` returns a
`VariantArrayHandle`. Since this had to be corrected anyway, it was decided
to change `CoordinateSystem`'s `GetData` to also return a
`VariantArrayHandle`, although its typelist is set to just `vtkm::Vec3f`.

To try to still support old code that expects the deprecated behavior of
returning an `ArrayHandleVirtualCoordinates`, `CoordinateSystem::GetData`
actually returns a "hidden" subclass of `VariantArrayHandle` that
automatically converts itself to an `ArrayHandleVirtualCoordinates`. (A
deprecation warning is given if this is done.)

This approach to support deprecated code is not perfect. The returned value
for `CoordinateSystem::GetData` can only be used as an `ArrayHandle` if a
method is directly called on it or if it is cast specifically to
`ArrayHandleVirtualCoordinates` or its superclass. For example, if passing
it to a method argument typed as `vtkm::cont::ArrayHandle<T, S>` where `T`
and `S` are template parameters, then the conversion will fail.

To continue to support ease of use, `CoordinateSystem` now has a method
named `GetDataAsMultiplexer` that returns the data as an
`ArrayHandleMultiplexer`. This can be employed to quickly use the
`CoordinateSystem` as an array without the overhead of a `CastAndCall`.

