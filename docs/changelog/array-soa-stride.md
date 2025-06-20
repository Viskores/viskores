## Added ArrayHandleSOAStride

Added a new array type named `ArrayHandleSOAStride`. This new array type holds
each of its components in a separate array. Unlike `ArrayHandleSOA`,
`ArrayHandleSOAStride` represents each component as an `ArrayHandleStride`
rather than a basic array. This allows `ArrayHandleSOAStride` to represent
most arrays with values of a fixed vector length.

`ArrayHandleSOAStride` can be used in similar situations as
`ArrayHandleRecombineVec`. The difference is that `ArrayHandleSOAStride`
requires the Vec length to be determined at compile time. This has both good and
bad consequences. The bad part is that if the number of components can vary,
`ArrayHandleSOAStride` cannot represent the data well. However, if the number of
components is fixed, then each value in the array is represented with a simple
`Vec`, which means it can be used as a drop-in replacement for basic arrays.

`ArrayHandleSOAStride` is created as a backup array when trying to do a cast and
call when the size and type of values can be determined.
