# `ArrayHandleCompositeVector` simplified and made writable.

`ArrayHandleCompositeVector` is now easier to use, as its type has a more
straightforward definition: `ArrayHandleCompositeVector<Array1, Array2, ...>`.
Previously, a helper metaprogramming struct was needed to determine the type
of the array handle.

In addition, the new implementation supports both reading and writing, whereas
the original version was read-only.

Another notable change is that the `ArrayHandleCompositeVector` no longer
supports component extraction from the source arrays. While the previous version
could take a source array with a `vtkm::Vec` `ValueType` and use only a single
component in the output, the new version requires that all input arrays have
the same `ValueType`, which becomes the `ComponentType` of the output
`vtkm::Vec`.

When component extraction is needed, the classes `ArrayHandleSwizzle` and
`ArrayHandleExtractComponent` have been introduced to allow the previous
usecases to continue working efficiently.
