## Remove warnings for deprecated Thrust utilities

The use of Thrust function objects such as `thrust::less`, `thrust::equal_to`,
and `thrust::plus`, as well as utilities such as `thrust::distance`, is
deprecated. These are replaced with the more general `cuda::std` equivalents.
The Viskores code is updated to point to the latter classes and functions for
newer versions of Cuda.
