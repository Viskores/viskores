# Add a common API for CoordinateSystem to unload execution resources

We now offer the ability to unload execution memory from ArrayHandleVirtualCoordinate
and CoordinateSystem using the ReleaseResourcesExecution method.

Field now has a ReleaseResourcesExecution.

This commit also fixes a bug that ArrayTransfer of ArrayHandleVirtualCoordinate
does not release execution resources properly.

