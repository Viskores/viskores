# Added `ArrayHandleRuntimeVec` to specify vector sizes at runtime.

The new `ArrayHandleRuntimeVec` is a fancy `ArrayHandle` allows you to
specify a basic array of `Vec`s where the number of components of the `Vec`
are not known until runtime. (It can also optionally specify scalars.) The
behavior is much like that of `ArrayHandleGroupVecVariable` except that its
representation is much more constrained. This constrained representation
allows it to be automatically converted to an `ArrayHandleBasic` with the
proper `Vec` value type. This allows one part of code (such as a file
reader) to create an array with any `Vec` size, and then that array can be
fed to an algorithm that expects an `ArrayHandleBasic` of a certain value
type.
