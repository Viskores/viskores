# Disable asserts for HIP architecture builds

`assert` is supported on recent HIP cards, but compiling it is very slow,
as it triggers the usage of `printf` which. Currently (ROCm 3.7) `printf`
has a severe performance penalty and should be avoided when possible.
By default, the `VTKM_ASSERT` macro has been disabled whenever compiling
for a HIP device via kokkos.

Asserts for HIP devices can be turned back on by turning the
`VTKm_NO_ASSERT_HIP` CMake variable off. Turning this CMake variable off
will enable assertions in HIP kernels unless there is another reason
turning off all asserts (such as a release build).
