## Support threaded partitioned histograms

`FieldRangeGlobalCompute` can now compute ranges for local partitions concurrently before reducing
them across MPI ranks. Partitioned `Histogram` uses this common implementation and initializes its
shared bin width before concurrently processing partitions, allowing it to safely use the standard
multithreaded filter settings.
