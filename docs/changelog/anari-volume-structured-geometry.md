## Fix ANARI volume origin and spacing on structured grids

`ANARIMapperVolume` computed the `structuredRegular` origin and spacing from the
coordinate bounds as if every sample sat on a point. For a cell field this
stretched the volume, and an axis with a single sample divided by zero. The
mapper now reads the origin and spacing from the uniform point coordinates and,
for a cell field, moves the origin half a spacing in so the samples sit at the
cell centers. The mapper used to render structured cell sets with rectilinear or
curvilinear coordinates as if they were uniform. It now rejects them with a
warning until they are supported.
