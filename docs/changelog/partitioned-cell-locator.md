# New partitioned cell locator class

A new version of a locator, `CellLocatorParitioned`, is now available. This version of a
locator takes a `PartitionedDataSet` and builds a structure that will find the partition Ids and
cell Ids for the input array of locations. It runs CellLocatorGeneral for each partition. We 
expect multiple hits and only return the first one (lowest partition Id) where the detected cell 
is of type REGULAR (no ghost, not blanked) in the vtkGhostType array. If this array does not 
exist in a partition, we assume that all cells are regular.

vtkm::cont::CellLocatorPartitioned produces an Arrayhandle of the size of the number of 
partitions filled with the execution objects of CellLocatorGeneral. It further produces an 
Arrayhandle filled with the ReadPortals of the vtkGhost arrays to then select the non-blanked 
cells from the potentially multiple detected cells on the different partitions. Its counterpart 
on the exec side, vtkm::exec::CellLocatorPartitioned, contains the actual FindCell function.
