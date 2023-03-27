# New Statistics filter

The statistics filter computes the descriptive statistics of the fields specified by users based on `DescriptiveStatistics`. Users can set `RequiredStatsList` to specify which statistics will be stored in the output data set. The statistics filter supports the distributed memory case based on the vtkmdiy, and the process with rank 0 will return the correct final reduced results.

