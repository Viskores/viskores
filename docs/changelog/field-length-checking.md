# Check to make sure that the fields in a DataSet are the proper length

It is possible in a `DataSet` to add a point field (or coordinate system)
that has a different number of points than reported in the cell set.
Likewise for the number of cells in cell fields. This is very bad practice
because it is likely to lead to crashes in worklets that are expecting
arrays of an appropriate length.

Although `DataSet` will still allow this, a warning will be added to the
VTK-m logging to alert users of the inconsistency introduced into the
`DataSet`. Since warnings are by default printed to standard error, users
are likely to see it.
