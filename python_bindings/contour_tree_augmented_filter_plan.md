# ContourTreeAugmented Output Refactor Plan

This note captures the proposed long-term cleanup for the
`ContourTreeAugmented` filter and Python bindings. It is intentionally a
planning note only; no implementation work is part of this change.

## Goal

Turn `viskores::filter::scalar_topology::ContourTreeAugmented` into a real
data-producing filter whose output `DataSet` contains the contour-tree products
needed by downstream consumers. Split volume branch decomposition into a
separate filter that consumes that output, following the architecture already
used by `ContourTreeUniformDistributed` and
`DistributedBranchDecompositionFilter`.

## Architectural Direction

The distributed contour-tree path is the right template:

- `ContourTreeUniformDistributed` materializes tree data as `WholeDataSet`
  fields.
- `DistributedBranchDecompositionFilter` consumes those fields and appends
  branch-decomposition data.

For the serial augmented contour tree, use the same separation of concerns, but
do not blindly expose all internal contour-tree arrays. Prefer a small, stable
field contract based on what branch decomposition and users actually need.

## Minimal ContourTreeAugmented Output

If the contour-tree filter computes volume weights before returning, the
separate branch-decomposition filter should only need these fields:

- `Supernodes`
- `Superarcs`
- `IntrinsicVolume`
- `DependentVolume`

The branch filter also needs total vertex count. Prefer deriving that from the
input/output cell set when available. If that is fragile, add a scalar metadata
field such as `TotalVolume`.

Useful user-facing fields to consider:

- `PointSuperarcIds`: point field indexed by original grid point id, mapping
  each grid point to its owning superarc.
- CSR-style `SuperarcPointOffsets` and `SuperarcPointIds` can be derived from
  `PointSuperarcIds` when needed. Prefer keeping the filter output to one
  direction of the point/superarc mapping unless a downstream consumer requires
  the grouped representation directly.
- For Python, derived membership helpers should live outside the top-level
  filter namespace, for example
  `viskores.filter.scalar_topology.helper.group_points_by_superarc`. Return an
  `ArrayHandleGroupVecVariableId` ragged handle rather than exposing offsets
  and point ids as the default user-facing shape.
- `SupernodePointIds` or a similarly named field if users need endpoint point
  ids without understanding sorted-index space.
- `DataValues` only if downstream branch outputs need endpoint values without
  re-reading the original input field.

## Fields To Keep Internal By Default

These arrays are useful internally, but should not be part of the default public
field contract unless a concrete consumer requires them:

- `SortOrder`: needed to translate sorted ids to original mesh ids, but users
  should receive semantic point ids instead.
- `Nodes`: used to group vertices by superarc while computing intrinsic volume;
  expose membership through `PointSuperarcIds` or CSR fields instead.
- `Arcs`: regular-arc construction detail, not needed for volume branch
  decomposition.
- `WhenTransferred`: not directly needed by the current serial volume-weight
  path.
- `FirstSupernodePerIteration` and `FirstHypernodePerIteration`: needed while
  computing dependent volume, but not needed once `DependentVolume` is in the
  output dataset.

The distributed branch-decomposition path exposes additional fields such as
`RegularNodeSortOrder`, `Regular2Supernode`, `WhichRound`, `NumRounds`, and
first-supernode-per-iteration components because it has to reconcile
hierarchical rounds across blocks. Treat those as distributed-specific
bookkeeping, not as the default serial augmented filter contract.

## Proposed Work Packages

### MR 1: Data-Producing ContourTreeAugmented

- Make `ContourTreeAugmented::DoExecute` return a dataset with stable contour
  tree fields rather than relying on filter object state.
- Compute and output `IntrinsicVolume` and `DependentVolume` when full
  augmentation is available.
- Add user-facing superarc membership fields if needed by Python workflows.
- Keep existing object-state accessors temporarily for compatibility.
- Add C++ tests validating output field presence, sizes, and consistency.
- Update Python tests to read fields from the output dataset.

### MR 2: Separate Serial Volume Branch-Decomposition Filter

- Add a new filter that consumes the `ContourTreeAugmented` output dataset.
- Have it read `Supernodes`, `Superarcs`, `IntrinsicVolume`, and
  `DependentVolume`.
- Have it append branch output fields, for example:
  `WhichBranch`, `BranchMinimum`, `BranchMaximum`, `BranchSaddle`,
  `BranchParent`.
- Bind the new filter in Python.
- Keep current Python convenience methods as wrappers or compatibility shims
  until the new workflow is established.

### MR 3: Cleanup And Alignment

- Remove or de-emphasize Python APIs that expose contour-tree filter internals.
- Align field names with the distributed filter where the concepts genuinely
  match.
- Document flagged ids such as `NO_SUCH_ELEMENT` and `IS_ASCENDING` only where
  they remain visible.
- Revisit whether `DataValues`, endpoint ids, or membership CSR fields belong in
  the stable public contract.

## Effort Estimate

- MR 1: 4 to 6 engineering days.
- MR 2: 4 to 7 engineering days.
- Python bindings, examples, and tests across both MRs: 2 to 3 days.
- Review hardening and CI fallout: 2 to 4 days.

Total expected effort for a clean Python-facing conversion is roughly 10 to 16
engineering days. A narrower C++-only proof of concept would likely be smaller,
but the stable field contract, Python workflow, and tests are the important part
of the cleanup.
