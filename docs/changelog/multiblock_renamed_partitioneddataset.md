# `MultiBlock` renamed to `PartitionedDataSet`

The `MultiBlock` class has been renamed to `PartitionedDataSet`, and its API
has been refactored to refer to "partitions", rather than "blocks".
Additionally, the `AddBlocks` method has been changed to `AppendPartitions` to
more accurately reflect the operation performed. The associated
`AssignerMultiBlock` class has also been renamed to
`AssignerPartitionedDataSet`.

This change is motivated towards unifying VTK-m's data model with VTK. VTK has
started to move away from `vtkMultiBlockDataSet`, which is a hierarchical tree
of nested datasets, to `vtkPartitionedDataSet`, which is always a flat vector
of datasets used to assist geometry distribution in multi-process environments.
This simplifies traversal during processing and clarifies the intent of the
container: The component datasets are partitions for distribution, not
organizational groupings (e.g. materials).
