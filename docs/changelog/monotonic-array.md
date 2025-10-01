## Monotonicty checks for arrays

Viskores has two new functions to test wheter a scalar-valued array is monotonically increasing or monotonically decreasing.
`viskores::cont::ArrayIsMonotonicIncreasing` returns `true` if the array is monotincally increasing.
`viskores::cont::ArrayIsMonotonicDecreasing` returns `true` if the array is monotincally decreasing.

Both functions use `viskores::cont::UnknownArrayHandle` as the input argument so that it can support any type of underlying array storage.
