# CMake vtkm_add_target_information() makes using vtk-m easier

This higher order function allow build-systems that use VTK-m
to use `add_library` or `add_executable` calls but still have an
easy to way to get the required information to have VTK-m using
compilation units compile correctly.

```cmake
 vtkm_add_target_information(
   target[s]
   [ DROP_UNUSED_SYMBOLS ]
   [ MODIFY_CUDA_FLAGS ]
   [ EXTENDS_VTKM ]
   [ DEVICE_SOURCES <source_list> ]
   )
```
 Usage:
 ```cmake
   add_library(lib_that_uses_vtkm STATIC a.cxx)
   vtkm_add_target_information(lib_that_uses_vtkm
                               MODIFY_CUDA_FLAGS
                               DEVICE_SOURCES a.cxx
                               )
   target_link_libraries(lib_that_uses_vtkm PRIVATE vtkm_filter)
```

## Options to vtkm_add_target_information

  - DROP_UNUSED_SYMBOLS: If enabled will apply the appropiate link
  flags to drop unused VTK-m symbols. This works as VTK-m is compiled with
  -ffunction-sections which allows for the linker to remove unused functions.
  If you are building a program that loads runtime plugins that can call
  VTK-m this most likely shouldn't be used as symbols the plugin expects
  to exist will be removed.
  Enabling this will help keep library sizes down when using static builds
  of VTK-m as only the functions you call will be kept. This can have a
  dramatic impact on the size of the resulting executable / shared library.
  - MODIFY_CUDA_FLAGS: If enabled will add the required -arch=<ver> flags
  that VTK-m was compiled with. If you have multiple libraries that use
  VTK-m calling `vtkm_add_target_information` multiple times with
  `MODIFY_CUDA_FLAGS` will cause duplicate compiler flags. To resolve this issue
  you can; pass all targets and sources to a single `vtkm_add_target_information`
  call, have the first one use `MODIFY_CUDA_FLAGS`, or use the provided
  standalone `vtkm_get_cuda_flags` function.

  - DEVICE_SOURCES: The collection of source files that are used by `target(s)` that
  need to be marked as going to a special compiler for certain device adapters
  such as CUDA.

  - EXTENDS_VTKM: Some programming models have restrictions on how types can be used,
  passed across library boundaries, and derived from.
  For example CUDA doesn't allow device side calls across dynamic library boundaries,
  and requires all polymorphic classes to be reachable at dynamic library/executable
  link time.
  To accommodate these restrictions we need to handle the following allowable
  use-cases:
    - Object library: do nothing, zero restrictions
    - Executable: do nothing, zero restrictions
    - Static library: do nothing, zero restrictions
    - Dynamic library:
      - Wanting to use VTK-m as implementation detail, doesn't expose VTK-m
      types to consumers. This is supported no matter if CUDA is enabled.
      - Wanting to extend VTK-m and provide these types to consumers.
      This is only supported when CUDA isn't enabled. Otherwise we need to ERROR!
      - Wanting to pass known VTK-m types across library boundaries for others
      to use in filters/worklets. This is only supported when CUDA isn't enabled. Otherwise we need to ERROR!

    For most consumers they can ignore the `EXTENDS_VTKM` property as the default will be correct.

The `vtkm_add_target_information` higher order function leverages the `vtkm_add_drop_unused_function_flags` and
`vtkm_get_cuda_flags` functions which can be used by VTK-m consuming applications.

The `vtkm_add_drop_unused_function_flags` function implements all the behavior of `DROP_UNUSED_SYMBOLS` for a single
target.

The `vtkm_get_cuda_flags` function implements a general form of `MODIFY_CUDA_FLAGS` but instead of modiyfing
the `CMAKE_CUDA_FLAGS` it will add the flags to any variable passed to it.



