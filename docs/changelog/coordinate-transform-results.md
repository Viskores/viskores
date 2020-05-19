# Result DataSet of coordinate transform has its CoordinateSystem changed

When you run one of the coordinate transform filters,
`CylindricalCoordinateTransform` or `SphericalCoordinateTransform`, the
transform coordiantes are placed as the first `CoordinateSystem` in the
returned `DataSet`. This means that after running this filter, the data
will be moved to this new coordinate space.

Previously, the result of these filters was just placed in a named `Field`
of the output. This caused some confusion because the filter did not seem
to have any effect (unless you knew to modify the output data). Not using
the result as the coordinate system seems like a dubious use case (and not
hard to work around), so this is much better behavior.
