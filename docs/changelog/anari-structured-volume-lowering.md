## Make ANARI structured-volume lowering truthful

`ANARIMapperVolume` now creates `structuredRegular` spatial fields only for
three-dimensional structured grids with uniform point coordinates, a non-empty
point-associated scalar field, matching point and field dimensions, and at
least two samples per dimension. Origin and spacing come directly from the
uniform coordinate representation rather than reconstructed coordinate bounds.

Nonuniform coordinates, cell-associated fields, vector fields, empty fields,
and degenerate dimensions are rejected with `ErrorBadValue`. The mapper also
checks required ANARI extensions and object subtypes before object creation.
Spatial-field handles and their backing arrays are replaced as one atomic
lifetime state, so failed actor replacements preserve the previous volume.
