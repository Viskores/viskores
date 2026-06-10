## Fix array access for rectilinear splines

The rectilinear spline execution object needs to know the
bounds of the coordinates. These were computed by loading
the first and last items of the coordinate arrays. However,
these arrays are on the device and the bounds are computed
on the host. Thus, for devices with different memory spaces
this could give bad values.

Fix the problem by using the `GetBounds` method on the
`CoordinateSystem` object, which will compute it correctly.
