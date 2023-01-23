# Added ability to resize strided arrays from ArrayExtractComponent

Previously, it was not possible to resize an `ArrayHandleStride` because
the operation is a bit ambiguous. The actual array is likely to be padded
by some amount, and there could be an unknown amount of space skipped at
the beginning.

However, there is a good reason to want to resize `ArrayHandleStride`. This
is the array used to implement the `ArrayExtractComponent` feature, and
this in turn is used when extracting arrays from an `UnknownArrayHandle`
whether independent or as an `ArrayHandleRecombineVec`.

The problem really happens when you create an array of an unknown type in
an `UnknownArrayHandle` (such as with `NewInstance`) and then use that as
an output to a worklet. Sure, you could use `ArrayHandle::Allocate` to
resize before getting the array, but that is awkward for programers.
Instead, allow the extracted arrays to be resized as normal output arrays
would be.
