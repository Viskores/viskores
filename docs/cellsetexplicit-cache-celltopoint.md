# CellSetExplicit now caches CellToPoint table when used with Invoke.

Issue #268 highlighted an issue where the expensive CellToPoint table
update was not properly cached when a CellSetExplicit was used with a
filter. This has been corrected by ensuring that the metadata
associated with the table survives shallow copying of the CellSet.

New methods are also added to check whether the CellToPoint table
exists, and also to reset it if needed (e.g. for benchmarking):

```
vtkm::cont::CellSetExplicit<> cellSet = ...;
// Check if the CellToPoint table has already been computed:
if (cellSet.HasConnectivity(vtkm::TopologyElementTagCell{},
                            vtkm::TopologyElementTagPoint{}))
{
  // Reset it:
  cellSet.ResetConnectivity(vtkm::TopologyElementTagCell{},
                            vtkm::TopologyElementTagPoint{});
}
```
