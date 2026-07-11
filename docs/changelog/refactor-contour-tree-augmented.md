## ContourTreeAugmented returns results via the output DataSet; deprecates the multi-block path

`ContourTreeAugmented` now returns its results as fields on the output
`DataSet` when run on a single `DataSet`: `Supernodes` and `Superarcs`
(whole-dataset fields), and, when the tree is augmented, `Superparents` (a
point field mapping each mesh vertex to its superarc). Enable
`SetComputeBranchDecomposition(true)` to additionally get `WhichBranch`,
`BranchMinimum`, `BranchMaximum`, `BranchSaddle`, and `BranchParent`. None of
these fields need a separate sort order to interpret: they are indexed by
mesh vertex or by supernode directly.

For downstream algorithms that operate in the sorted space the tree was
computed in, `SetIncludeSortOrder(true)` additionally exports the `SortOrder`
field, the permutation listing the mesh vertices in (value, mesh index)
lexicographic order. Inverting this permutation with a single scatter
reconstructs sorted-space views of all outputs without re-sorting the data.
This keeps the full flexibility of the previous sorted-space results
available (for example for a future filter that computes the branch
decomposition from the contour tree output), while mesh-vertex indexing
remains the single canonical form of the output arrays.

The augmentation level is now a simple boolean: use `SetAugmentTree(bool)` /
`GetAugmentTree()` in place of the constructor's `computeRegularStructure`
argument. `SetComputeBranchDecomposition(true)` requires an augmented tree;
if augmentation isn't already enabled, `Execute()` enables it automatically
and logs a warning, regardless of the order you called the setters in.
`GetContourTreeStatistics()` returns array sizes and per-iteration
hyperstructure statistics of the computed tree as a `ContourTreeStatistics`
object that can be converted to a loggable string via `ToString()` or
`operator<<`.

The multi-block (`PartitionedDataSet`/MPI) path of `ContourTreeAugmented` is
now deprecated. Computing a monolithic contour tree for the entire data set
is inherently inefficient in parallel, which is why Viskores switched to the
distributed tree representation of `ContourTreeUniformDistributed`; boundary
augmentation was only a way to make the monolithic computation somewhat
efficient. The multi-block path was largely kept for performance comparisons
and because a full analysis pipeline for the distributed case did not exist
at the time. That pipeline exists now: use `ContourTreeUniformDistributed`
together with `DistributedBranchDecompositionFilter`,
`SelectTopVolumeBranchesDistributedFilter`, and
`ExtractTopVolumeContoursFilter`. The distributed pipeline is not a 1:1
replacement, but new applications should extend it rather than build on a
representation that scales poorly. The deprecated path keeps its established
behavior unchanged (results reachable via `GetContourTree()`,
`GetSortOrder()`, and `GetNumIterations()`).

`SelectTopVolumeBranchesFilter` is renamed
`SelectTopVolumeBranchesDistributedFilter` to reflect that it belongs to the
distributed pipeline; the old name is kept as a deprecated alias. This is
also in preparation for a future PR that splits the non-distributed branch
decomposition computation out of `ContourTreeAugmented` into a separate
filter.

`viskores::worklet::contourtree_augmented::ProcessContourTree` gained helpers
that operate directly on a `ContourTreeAugmented` output `DataSet`:
`CollectSortedSuperarcs`, `CollectRegularVerticesPerSuperarc`,
`SelectTopVolumeBranches`, and `ComputeBranchDecomposition` overloads that
work in mesh vertex space (no sort order needed). A new minimal example,
`contour_tree_augmented_simple`, demonstrates the new API end-to-end and
writes GraphViz `.dot` files for both the contour tree (superarcs labeled
with their regular vertices) and its branch decomposition (branches shown in
color).
