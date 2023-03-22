# Added a reader for VisIt files.

A VisIt file is a text file that contains the path and filename of a number of VTK files. This provides a convenient way to load `vtkm::cont::PartitionedDataSet` data from VTK files. The first line of the file is the keyword `!NBLOCKS <N>` that specifies the number of VTK files to be read.
