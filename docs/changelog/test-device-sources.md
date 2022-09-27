# Added DEVICE_SOURCES to vtkm_unit_tests

The `vtkm_unit_tests` function in the CMake build now allows you to specify
which files need to be compiled with a device compiler using the
`DEVICE_SOURCES` argument. Previously, the only way to specify that unit
tests needed to be compiled with a device compiler was to use the
`ALL_BACKENDS` argument, which would automatically compile everything with
the device compiler as well as test the code on all backends.
`ALL_BACKENDS` is still supported, but it no longer changes the sources to
be compiled with the device compiler.
