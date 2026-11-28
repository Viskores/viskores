## MAGIC Gaussian Isosurface Uncertainty Filters

Viskores now provides filters to compute the positional uncertainty of
isosurfaces when input scalar data are uncertain and assumed to follow a
Gaussian distribution. Two uncertainty models are provided:

`ContourUncertainGaussianIndependent` computes isosurface crossing uncertainty
for data where the Gaussian distributions at each grid point are independent.
The filter takes a mean field and a pointwise variance field as input and
dispatches to `ContourUncertainMAGICIndependentClosedForm` or
`ContourUncertainMAGICIndependentMonteCarlo` according to `SetApproach`. The
two implementation filters may also be used directly.

`ContourUncertainGaussianCorrelated` extends the independent case by accounting
for spatial correlation between neighboring grid points. It additionally takes
per-axis edge covariance fields (RhoX, RhoY, RhoZ) and requires a structured
3D grid as input. It dispatches to
`ContourUncertainMAGICCorrelatedClosedForm` or
`ContourUncertainMAGICCorrelatedMonteCarlo` according to `SetApproach`; both
implementation filters can also be used directly.

The independent dispatcher and correlated filter support two algorithms,
selectable via `SetApproach`: a closed-form solution (the default, based on the
Hinkley/Marsaglia ratio derivation) and a Monte Carlo approach that samples the
joint Gaussian distribution, with the per-edge sample count set by
`SetNumberOfSamples`.

Both filters output the expected isosurface vertex positions and a
variance field representing the uncertainty of those positions. The
implementation is based on the MAGIC algorithm (Athawale et al., 2025).
