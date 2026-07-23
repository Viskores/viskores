## Preserve coordinate precision in unstructured cell locators

`CellLocatorUniformBins`, `CellLocatorTwoLevel`, and
`CellLocatorBoundingIntervalHierarchy` now perform point-in-cell tests with the
native `Float32` or `Float64` coordinate array instead of casting all
coordinates to `FloatDefault`. This prevents cells with `Float64` boundaries
that are not representable in `Float32` from producing false-positive matches.
