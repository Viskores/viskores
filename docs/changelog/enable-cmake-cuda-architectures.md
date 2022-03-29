## Enable CMAKE_CUDA_ARCHITECTURES

When using _CMake_ > 3.18, `CMAKE_CUDA_ARCHITECTURES` can now be used instead of
`VTKm_CUDA_Architecture` to specify the list of architectures desired for the
compilation of _CUDA_ sources. 

Since `CMAKE_CUDA_ARCHITECTURES` is the canonical method of specifying _CUDA_
architectures in _CMake_ and it is more flexible, for instance we can also
specify _CUDA_ virtual architectures, from _CMake_ 3.18 explicitly setting
`VTKm_CUDA_Architecture` will be deprecated whilst still supported.
