## Add extrusion filters

Added `viskores::filter::geometry_refinement::ExtrusionLinear` and
`viskores::filter::geometry_refinement::ExtrusionRotational`, which sweep a
triangulated profile to produce wedge cells. The filters support configurable
plane count, optional input triangulation, compact or explicit output topology,
and point/cell field mapping from the input profile to the generated volume.
Both filters share `ExtrusionAbstract` as the common base for shared extrusion
controls and execution setup.

`ExtrusionLinear` translates the profile along a direction, and
`ExtrusionRotational` revolves the profile around an axis with configurable
center, sweep angle, closed sweeps, and axis-point validation. Full `2*pi`
rotational sweeps default to closed unless explicitly overridden.
