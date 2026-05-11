## Remove warnings for deprecated Thrust function objects

The use of Thrust function objects such as `thrust::less`, `thrust::equal_to`,
and `thrust::plus` are deprecated. These are replaced with the more general
`cuda::std` equivalents. The Viskores code is updated to point to the latter
classes for newer versions of Cuda.
