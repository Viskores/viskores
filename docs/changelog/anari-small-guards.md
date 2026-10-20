## ANARI mappers guard a zero-width color map range and bad field indices

`SetANARIColorMapValueRange` on the ANARI triangles and points mappers used to
divide by the width of the range, so a range whose minimum equals its maximum
gave the color map sampler an infinite transform and miscolored the surface.
Such a range is now widened symmetrically about its value, which maps to the
middle of the color map. `ANARIActor::SetPrimaryFieldIndex` now throws
`viskores::cont::ErrorBadValue` for an index outside 0-3, and
`ANARIActor::GetField` throws for an index above 3, instead of reading past the
actor's four fields.
