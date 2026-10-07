## Use a global histogram for partitioned histogram sampling

`HistSampling` now builds one histogram across all partitions and MPI ranks when run on a
`PartitionedDataSet`. Sampling probabilities therefore reflect global value frequencies instead
of treating each partition independently.
