# Enable new instances of unknown arrays with dynamic sizes

`UnknownArrayHandle` allows you to create a new instance of a compatible
array so that when receiving an array of unknown type, a place to put the
output can be created. However, these methods only worked if the number of
components in each value could be determined statically at compile time.

However, there are some special `ArrayHandle`s that can define the number
of components at runtime. In this case, the `ArrayHandle` would throw an
exception if `NewInstanceBasic` or `NewInstanceFloatBasic` was called.

Although rare, this condition could happen when, for example, an array was
extracted from an `UnknownArrayHandle` with `ExtractArrayFromComponents` or
with `CastAndCallWithExtractedArray` and then the resulting array was
passed to a function with arrays passed with `UnknownArrayHandle` such as
`ArrayCopy`.
