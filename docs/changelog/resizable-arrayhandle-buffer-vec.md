# Allow ArrayHandle to have a runtime selectable number of buffers

Previously, the number of buffers held by an `ArrayHandle` had to be
determined statically at compile time by the storage. Most of the time this
is fine. However, there are some exceptions where the number of buffers
need to be selected at runtime. For example, the `ArrayHandleRecombineVec`
does not specify the number of components it uses, and it needed a hack
where it stored buffers in the metadata of another buffer, which is bad.

This change allows the number of buffers to vary at runtime (at least at
construction). The buffers were already managed in a `std::vector`. It now
no longer forces the vector to be a specific size. `GetNumberOfBuffers` was
removed from the `Storage`. Instead, if the number of buffers was not
specified at construction, an allocation of size 0 is done to create
default buffers.

The biggest change is to the interface of the storage object methods, which
now take `std::vector` instead of pointers to `Buffer` objects. This adds a
little hassle in having to copy subsets of this `vector` when a storage
object has multiple sub-arrays. But it does simplify some of the
templating.

Other changes to the `Storage` structure include requiring all objects to
include a `CreateBuffers` method that accepts no arguments. This method
will be used by `ArrayHandle` in its default constructor. Previously,
`ArrayHandle` would create the `vector` of `Buffer` objects itself, but it
now must call this method in the `Storage` to do this. (It also has a nice
side effect of allowing the `Storage` to initialize the buffer objects if
necessary. Another change was to remove the `GetNumberOfBuffers` method
(which no longer has meaning).
