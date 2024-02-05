# Support `Fill` for `ArrayHandleStride`

Previously, if you called `Fill` on an `ArrayHandleStride`, you would get
an exception that said the feature was not supported. It turns out that
filling values is very useful in situations where, for example, you need to
initialize an array when processing an unknown type (and thus dealing with
extracted components).

This implementation of `Fill` first attempts to call `Fill` on the
contained array. This only works if the stride is set to 1. If this does
not work, then the code leverages the precompiled `ArrayCopy`. It does this
by first creating a new `ArrayHandleStride` containing the fill value and a
modulo of 1 so that value is constantly repeated. It then reconstructs an
`ArrayHandleStride` for itself with a modified size and offset to match the
start and end indices.

Referencing the `ArrayCopy` was tricky because it kept creating circular
dependencies among `ArrayHandleStride`, `ArrayExtractComponent`, and
`UnknownArrayHandle`. These dependencies were broken by having
`ArrayHandleStride` directly use the internal `ArrayCopyUnknown` function
and to use a forward declaration of `UnknownArrayHandle` rather than
including its header.
