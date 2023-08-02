# Updated the interface and documentation of GhostCellRemove

The `GhostCellRemove` filter had some methods inconsistent with the naming
convention elsewhere in VTK-m. The class itself was also in need of some
updated documentation. Both of these issues have been fixed.

Additionally, there were some conditions that could lead to unexpected
behavior. For example, if the filter was asked to remove only ghost cells
and a cell was both a ghost cell and blank, it would not be removed. This
has been updated to be more consistent with expectations.
