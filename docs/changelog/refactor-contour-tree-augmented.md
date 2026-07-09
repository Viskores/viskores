## ContourTreeAugmented returns results via the output DataSet; deprecates the multi-block path

`ContourTreeAugmented` now returns its results as fields on the output
`DataSet` when run on a single `DataSet`: `Supernodes` and `Superarcs`
(whole-dataset fields), and — when the tree is augmented — `Superparents` (a
point field mapping each mesh vertex to its superarc). Enable
`SetComputeBranchDecomposition(true)` to additionally get `WhichBranch`,
`BranchMinimum`, `BranchMaximum`, `BranchSaddle`, and `BranchParent`. None of
these fields need a separate sort order to interpret — they are indexed by
mesh vertex or by supernode directly.

The augmentation level is now a simple boolean: use `SetAugmentTree(bool)` /
`GetAugmentTree()` in place of the constructor's `computeRegularStructure`
argument and the deprecated `SetComputeRegularStructure`/
`GetComputeRegularStructure`. `SetComputeBranchDecomposition(true)` requires
an augmented tree; if augmentation isn't already enabled, `Execute()` enables
it automatically and logs a warning, regardless of the order you called the
setters in.

The multi-block (`PartitionedDataSet`/MPI) path of `ContourTreeAugmented` is
now deprecated. Full augmentation of a contour tree computed in parallel is
inherently inefficient, which is why that path historically only supported a
cheaper boundary-only augmentation — but that mode cannot be expressed in the
new mesh-vertex-indexed output. If you compute contour trees over multi-block
data, switch to `ContourTreeUniformDistributed` together with
`DistributedBranchDecompositionFilter`, `SelectTopVolumeBranchesDistributedFilter`,
and `ExtractTopVolumeContoursFilter`. The deprecated path keeps its established
behavior unchanged (results reachable via `GetContourTree()`, `GetSortOrder()`,
and `GetNumIterations()`).

`SelectTopVolumeBranchesFilter` is renamed `SelectTopVolumeBranchesDistributedFilter`
to reflect that it belongs to the distributed pipeline; the old name is kept
as a deprecated alias.

`viskores::worklet::contourtree_augmented::ProcessContourTree` gained helpers
that operate directly on a `ContourTreeAugmented` output `DataSet`:
`CollectSortedSuperarcs`, `CollectRegularVerticesPerSuperarc`,
`SelectTopVolumeBranches`, and `ComputeBranchDecompositionMeshSpace`. A new
minimal example, `contour_tree_augmented_simple`, demonstrates the new API
end-to-end and writes GraphViz `.dot` files for both the contour tree and its
branch decomposition.
