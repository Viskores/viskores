# VTK-m namespace for its exported CMake targets

VTK-m exported CMake targets are now prefixed with the `vtkm::` namespace.

## What it means for VTK-m users

VTK-m users will now need to prepend a `vtkm::` prefix when they refer to a
VTK-m CMake target in their projects as shown below:

```
add_executable(example example.cxx)
# Before:
target_link_libraries(example vtkm_cont vtkm_rendering)
# Now:
target_link_libraries(example vtkm::cont vtkm::rendering)
```

For compatibility purposes we still provide additional exported targets with the
previous naming scheme, in the form of `vtkm_TARGET`,  when VTK-m is found
using:

```
# With any version less than 2.0
find_package(VTK-m 1.9)

add_executable(example example.cxx)
# This is still valid
target_link_libraries(example vtkm_cont vtkm_rendering)
```

Use with care since we might remove those targets in future releases.

## What it means for VTK-m developers

While VTK-m exported targets are now prefixed with the `vtkm::` prefix, internal
target names are still in the form of `vtkm_TARGET`.

To perform this name transformation in VTK-m targets a new CMake function has
been provided that decorates the canonical `install` routine. Use this functions
instead of `install` when creating new `VTK-m` targets, further information can
be found at the `vtkm_install_targets` function header at
`CMake/VTKmWrappers.cmake`.
