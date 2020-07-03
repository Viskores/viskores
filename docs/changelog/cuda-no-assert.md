# Disable asserts for CUDA architecture builds

`assert` is supported on recent CUDA cards, but compiling it appears to be
very slow. By default, the `VTKM_ASSERT` macro has been disabled whenever
compiling for a CUDA device (i.e. when `__CUDA_ARCH__` is defined).

Asserts for CUDA devices can be turned back on by turning the
`VTKm_NO_ASSERT_CUDA` CMake variable off. Turning this CMake variable off
will enable assertions in CUDA kernels unless there is another reason
turning off all asserts (such as a release build).
