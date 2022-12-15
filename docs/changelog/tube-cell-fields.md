# Fix handling of cell fields in Tube filter

The `Tube` filter wraps a tube of polygons around poly line cells.
During this process it had a strange (and wrong) handling of cell data.
It assumed that each line had an independent field entry for each
segment of each line. It thus had lots of extra code to find the length
and offsets of the segment data in the cell data.

This is simply not how cell fields work in VTK-m. In VTK-m, each cell
has exactly one entry in the cell field array. Even if a polyline has
100 segments, it only gets one cell field value. This behavior is
consistent with how VTK treats cell field arrays.

The behavior the `Tube` filter was trying to implement was closer to an
"edge" field. However, edge fields are currently not supported in VTK-m.
The proper implementation would be to add edge fields to VTK-m. (This
would also get around some problems with the implementation that was
removed here when mixing polylines with other cell types and degenerate
lines.)
