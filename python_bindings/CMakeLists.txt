##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

# Keep stable ABI support optional while the binding surface is small. This
# lets developers check abi3 compatibility without making it the default build.
option(VISKORES_PYTHON_STABLE_ABI "Build Python bindings with the CPython stable ABI" OFF)

set(Viskores_PYTHON_COMPONENTS Interpreter Development)
if (VISKORES_PYTHON_STABLE_ABI AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
  list(APPEND Viskores_PYTHON_COMPONENTS Development.SABIModule)
endif()
find_package(Python 3.8 REQUIRED COMPONENTS ${Viskores_PYTHON_COMPONENTS})

# Minimum nanobind version. nb::ndarray dtype dispatch and the stable-ABI
# support used here require nanobind 2.0 or newer.
set(Viskores_PYTHON_NANOBIND_MIN_VERSION "2.0.0")

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_VARIABLE Viskores_PYTHON_NANOBIND_CMAKE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE Viskores_PYTHON_NANOBIND_RESULT
)
if (NOT Viskores_PYTHON_NANOBIND_RESULT EQUAL 0)
  message(FATAL_ERROR
    "nanobind ${Viskores_PYTHON_NANOBIND_MIN_VERSION} or newer is required to "
    "build viskores_interop_python. Install Python dependencies with:\n"
    "pip install -r ${Viskores_SOURCE_DIR}/python_bindings/requirements.txt")
endif()

find_package(nanobind ${Viskores_PYTHON_NANOBIND_MIN_VERSION} CONFIG REQUIRED
  PATHS "${Viskores_PYTHON_NANOBIND_CMAKE_DIR}"
  NO_DEFAULT_PATH
)

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

# Minimum NumPy version. The binding uses the buffer protocol and the standard
# ndarray flags (c_contiguous, aligned, writeable) that have been stable since
# NumPy 1.20.
set(Viskores_PYTHON_NUMPY_MIN_VERSION "1.20.0")

# Pass the minimum version as an argv argument rather than substituting it into
# the Python source string, so an unexpected character in the version literal
# cannot interact with Python parsing. NumPy ships its own version comparator
# (numpy.lib.NumpyVersion), so we don't need the third-party `packaging` module.
execute_process(
  COMMAND "${Python_EXECUTABLE}" -c
    "import sys; from numpy.lib import NumpyVersion; import numpy; \
sys.exit(0 if NumpyVersion(numpy.__version__) >= NumpyVersion(sys.argv[1]) else 1)"
    "${Viskores_PYTHON_NUMPY_MIN_VERSION}"
  RESULT_VARIABLE Viskores_PYTHON_NUMPY_RESULT
)
if (NOT Viskores_PYTHON_NUMPY_RESULT EQUAL 0)
  message(FATAL_ERROR
    "NumPy ${Viskores_PYTHON_NUMPY_MIN_VERSION} or newer is required to build "
    "Viskores Python bindings.")
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
