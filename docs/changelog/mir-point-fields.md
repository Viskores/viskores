## Fix the interpolation of point fields in the MIR filter

`viskores::filter::contour::MIRFilter` now interpolates the point fields of
new points from all of their parent points. Before, each new point took the
value of its first parent point in release builds, and debug builds failed an
assert.
