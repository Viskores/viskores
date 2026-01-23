## Fiber Uncertainty Visualization Filter

Viskores now provides a function to compute the positional probability of fibers
when input bivariate scalar data are uncertain. Fibers are a conceptual extension
of univariate isosurfaces (Carr et al., 2015); they represent the preimage of a
user-specified bivariate trait, analogous to an isovalue in the univariate case.
The positional probability of fibers can be computed for uniformly distributed
uncertain data sampled on a regular grid using the FiberUncertainUniform filter.
The bounds of the uniform distribution are estimated from the input ensemble by
computing the minimum and maximum values of each variable.
