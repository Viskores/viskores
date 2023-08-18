# Fix new instances of ArrayHandleRuntimeVec in UnknownArrayHandle

`UnknownArrayHandle` is supposed to treat `ArrayHandleRuntimeVec` the same
as `ArrayHandleBasic`. However, the `NewInstance` methods were failing
because they need custom handling of the vec size. Special cases in the
`UnknownArrayHandle::NewInstance*()` methods have been added to fix this
problem.

