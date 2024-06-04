# Disable Thrust patch that is no longer needed in modern Thrust

There is a Thrust patch that works around an issue in Thrust 1.9.4
(https://github.com/NVIDIA/thrust/issues/972). The underlying issue
should be fixed in recent versions. In recent versions of CUDA, the patch
breaks (https://gitlab.kitware.com/vtk/vtk-m/-/issues/818).

This change fixes the problem by disabling the patch where it is not
needed.
