# Add new option to VTKm_CUDA_Architecture

A new VTKm_CUDA_Architecture option called 'none' has been added. This will
disable all VTK-m generated cuda architecture flags, allowing the user to
specify their own custom flags.

Useful when VTK-m is used as a library in another project and the project wants
to use its own architecture flags.
