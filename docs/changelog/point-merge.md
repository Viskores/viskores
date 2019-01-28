# Add point merge capabilities to CleanGrid filter

We have added a `PointMerge` worklet that uses a virtual grid approach to
identify nearby points. The worklet works by creating a very fine but
sparsely represented locator grid. It then groups points by grid bins and
finds those within a specified radius.

This functionality has been integrated into the `CleanGrid` filter. The
following flags have been added to `CleanGrid` to modify the behavior of
point merging.

  * `Set`/`GetMergePoints` - a flag to turn on/off the merging of
    duplicated coincident points. This extra operation will find points
    spatially located near each other and merge them together.
  * `Set`/`GetTolerance` - Defines the tolerance used when determining
    whether two points are considered coincident. If the
    `ToleranceIsAbsolute` flag is false (the default), then this tolerance
    is scaled by the diagonal of the points. This parameter is only used
    when merge points is on.
  * `Set`/`GetToleranceIsAbsolute` - When ToleranceIsAbsolute is false (the
     default) then the tolerance is scaled by the diagonal of the bounds of
     the dataset. If true, then the tolerance is taken as the actual
     distance to use. This parameter is only used when merge points is on.
  * `Set`/`GetFastMerge` - When FastMerge is true (the default), some
     corners are cut when computing coincident points. The point merge will
     go faster but the tolerance will not be strictly followed.
