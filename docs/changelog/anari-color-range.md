## Fixed color range for ANARI sampler

The ANARI device ignored the inTransform and inOffset parameters of the 1D
sampler. Instead, it scaled the incoming field by its computed range. However,
the ANARI device should rely on the the client to provide the proper transform
for the colormap lookup.

The device now does an inverse transform of the color range values 0 and 1 to
use as the effective minimum and maximum of the field range. This replicates the
scaling of the inTransform as long as it only transforms 1D values, which is all
that is supported anyway.
