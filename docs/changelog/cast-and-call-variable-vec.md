# Added support for `CastAndCallVariableVecField` in `FilterField`

The `FilterField` class provides convenience functions for subclasses to
determine the `ArrayHandle` type for scalar and vector fields. However, you
needed to know the specific size of vectors. For filters that support an
input field of any type, a new form, `CastAndCallVariableVecField` has been
added. This calls the underlying functor with an `ArrayHandleRecombineVec`
of the appropriate component type.

The `CastAndaCallVariableVecField` method also reduces the number of
instances created by having a float fallback for any component type that
does not satisfy the field types.
