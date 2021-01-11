# Recombine extracted component arrays from unknown arrays

Building on the recent capability to [extract component arrays from unknown
arrays](array-extract-component.md), there is now also the ability to
recombine these extracted arrays to a single `ArrayHandle`. It might seem
counterintuitive to break an `ArrayHandle` into component arrays and then
combine the component arrays back into a single `ArrayHandle`, but this is
a very handy way to run algorithms without knowing the exact `ArrayHandle`
type.

Recall that when extracting a component array from an `UnknownArrayHandle`
you only need to know the base component of the value type of the contained
`ArrayHandle`. That makes extracting a component array independent from
either the size of any `Vec` value type and any storage type.

The added `UnknownArrayHandle::ExtractArrayFromComponents` method allows
you to use the functionality to transform the unknown array handle to a
form of `ArrayHandle` that depends only on this base component type. This
method internally uses a new `ArrayHandleRecombineVec` class, but this
class is mostly intended for internal use by this class.
