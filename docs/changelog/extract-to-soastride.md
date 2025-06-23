## Extract Arrays to ArrayHandleSOAStride

`UnknownArrayHandle` has a new method named `ExtractArrayWithValueType` that
extracts an array with a given `ValueType` regardless of the storage. The method
extracts the data to an `ArrayHandleSOAStride` by extracting each component and
collecting them in that array type. This provides similar behavior to
`ExtractArrayFromComponents` except that it extracts to a `Vec` (or scalar) of
known type and size. This means that the array can later be used as a normal
array without the restrictions that an `ArrayHandleRecombineVec` that comes from
`ExtractArrayFromComponents` has.

Additionally, you can pull most arrays from an `UnknownArrayHandle` using
`AsArrayHandle` with an `ArrayHandleSOAStride` as long as the value types match.
A consequence of this is that if `StorageTagSOAStride` is added to the storage
types to check when doing a `CastAndCall`, then the cast and call will
automatically put the data into an array of that type. This makes for a
convenient backup for unexpected storage or a way to not worry about storage. As
such, `StorageTagSOAStride` has been added to the list default storage types to
check.
