##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

set(Viskores_PYTHON_NANOBIND_STABLE_ABI)
if (VISKORES_PYTHON_STABLE_ABI)
  if (Python_VERSION VERSION_LESS 3.12)
    message(FATAL_ERROR
      "VISKORES_PYTHON_STABLE_ABI requires Python 3.12 or newer.")
  endif()
  if (NOT TARGET Python::SABIModule)
    message(FATAL_ERROR
      "VISKORES_PYTHON_STABLE_ABI requires CMake FindPython support for "
      "Python::SABIModule. Use CMake 3.26 or newer, or configure with "
      "-DVISKORES_PYTHON_STABLE_ABI=OFF.")
  endif()
  set(Viskores_PYTHON_NANOBIND_STABLE_ABI STABLE_ABI)
endif()

set(Viskores_PYTHON_PACKAGE_ROOT "${Viskores_BINARY_DIR}/python_bindings")
set(Viskores_PYTHON_PACKAGE_DIR "${Viskores_PYTHON_PACKAGE_ROOT}/viskores")
file(MAKE_DIRECTORY "${Viskores_PYTHON_PACKAGE_DIR}")

# List Python source files explicitly. The set is small and listing files keeps
# the dependency graph deterministic; revisit if the package grows.
set(Viskores_PYTHON_PACKAGE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/viskores/__init__.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/viskores/cont/__init__.py"
)
set(Viskores_PYTHON_PACKAGE_STAMP
  "${Viskores_BINARY_DIR}/python_bindings/.python_package_sync.stamp")

add_custom_command(
  OUTPUT "${Viskores_PYTHON_PACKAGE_STAMP}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${Viskores_PYTHON_PACKAGE_ROOT}"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/viskores"
          "${Viskores_PYTHON_PACKAGE_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E touch "${Viskores_PYTHON_PACKAGE_STAMP}"
  DEPENDS ${Viskores_PYTHON_PACKAGE_FILES}
)
add_custom_target(viskores_python_package_sync
  DEPENDS "${Viskores_PYTHON_PACKAGE_STAMP}")

nanobind_add_module(viskores_python_bindings ${Viskores_PYTHON_NANOBIND_STABLE_ABI} NOMINSIZE
  src/cont_bindings.cxx
  src/viskores_python_bindings.cxx
)
target_compile_features(viskores_python_bindings PRIVATE cxx_std_17)
target_compile_definitions(viskores_python_bindings PRIVATE PY_SSIZE_T_CLEAN)
target_include_directories(viskores_python_bindings
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(viskores_python_bindings PRIVATE
  viskores_cont
  viskores_interop_python
)
add_dependencies(viskores_python_bindings viskores_python_package_sync)
set_target_properties(viskores_python_bindings PROPERTIES
  PREFIX ""
  OUTPUT_NAME "_viskores"
  LIBRARY_OUTPUT_DIRECTORY "${Viskores_PYTHON_PACKAGE_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${Viskores_PYTHON_PACKAGE_DIR}"
)

set(_viskores_python_site_dir
  "lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages")
install(TARGETS viskores_python_bindings
  LIBRARY DESTINATION "${_viskores_python_site_dir}/viskores"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/viskores/"
  DESTINATION "${_viskores_python_site_dir}/viskores"
  FILES_MATCHING PATTERN "*.py"
)
