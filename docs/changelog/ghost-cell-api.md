# Change name of method to set the cell ghost levels in a DataSet

Previously, the method was named `AddGhostCellField`. However, only one
ghost cell field can be marked at a time, so `SetGhostCellField` is more
appropriate.
