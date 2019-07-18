# Enable writing to ArrayHandleCast

Previously, `ArrayHandleCast` was considered a read-only array handle.
However, it is trivial to reverse the cast (now that `ArrayHandleTransform`
supports an inverse transform). So now you can write to a cast array
(assuming the underlying array is writable).

One trivial consequence of this change is that you can no longer make a
cast that cannot be reversed. For example, it was possible to cast a simple
scalar to a `Vec` even though it is not possible to convert a `Vec` to a
scalar value. This was of dubious correctness (it is more of a construction
than a cast) and is easy to recreate with `ArrayHandleTransform`.
