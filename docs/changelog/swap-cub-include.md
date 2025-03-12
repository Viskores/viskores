Fix include for cub::Swap

A problem we have with the `vtkm::Swap` method is that it can be
ambiguous with the `cub::Swap` method that is part of the CUDA CUB
library. We get around this problem by using the CUB version of the
function when it is available.

However, we were missing an include statement that necessarily provided
`cub::Swap`. This function is now explicitly provided so that we no
longer rely on including it indirectly elsewhere.
