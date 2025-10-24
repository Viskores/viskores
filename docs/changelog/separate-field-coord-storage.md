## Separated the storage types for fields and coordinate systems

The storage types that are typically used for coordinate systems have been
removed from `VISKORES_DEFAULT_STORAGE_LIST` and a new list named
`VISKORES_DEFAULT_STORAGE_LIST_COORDINATES` contains the storage types typical
for coordinate systems.

Although a coordinate system is really just a field with a special attribute,
there are special array types to represent structured coordinate systems that
make little sense elsewhere. Previously, these types were listed for all fields.
However, now there is a backup representation in `ArrayHandleSOAStride` that can
reasonably represent these specific types. When operating directly on cells with
a coordinate system, it may matter what the type of coordinate system array is
because, for example, operations like gradients and location are much faster in
axis-aligned coordinates. However, for general vector operations, there is
little benefit to operating on the direct array type.

To compensate for these two use cases, Viskores now provides separate storage
lists for these two cases.
