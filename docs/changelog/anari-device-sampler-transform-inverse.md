## ANARI device color maps accept wide value ranges

The Viskores ANARI device finds the field range of an `image1D` or `image2D`
sampler by inverting its `inTransform`. It inverted the single-precision
matrix with a pivot check against single-precision epsilon, so the transform
of any value range wider than about 8e6 was reported as not invertible and
the color map fell back to the range [0, 1]. The device now inverts the
scale and translation of each axis directly in double precision. It reports
a non-invertible transform only when a scale is zero or not finite, or the
resulting range is not finite, and then falls back to [0, 1] for that axis
only. The `inOffset` is now subtracted before the inverse scale rather than
after, as the sampler adds it after `inTransform`, and the second axis of an
`image2D` sampler uses its own scale and translation rather than those of the
first.
