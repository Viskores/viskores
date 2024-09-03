## Add a form of CellInterpolate that operates on whole cell sets

The initial implementation of `CellInterpolate` takes arguments that are
expected from a topology map worklet. However, sometimes you want to
interplate cells that are queried from locators or otherwise come from a
`WholeCellSet` control signature argument.

A new form of `CellInterpolate` is added to handle this case.
