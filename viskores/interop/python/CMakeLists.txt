##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

find_package(Python 3.8 REQUIRED COMPONENTS Interpreter Development)

set(Viskores_PYTHON_NANOBIND_MIN_VERSION "2.0.0")

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_VARIABLE _viskores_interop_python_nanobind_cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE _viskores_interop_python_nanobind_result
)
if(NOT _viskores_interop_python_nanobind_result EQUAL 0)
  message(FATAL_ERROR
    "nanobind ${Viskores_PYTHON_NANOBIND_MIN_VERSION} or newer is required to "
    "build viskores_interop_python. Install Python dependencies with:\n"
    "pip install -r ${Viskores_SOURCE_DIR}/python_bindings/requirements.txt")
endif()

find_package(nanobind ${Viskores_PYTHON_NANOBIND_MIN_VERSION} CONFIG REQUIRED
  PATHS "${_viskores_interop_python_nanobind_cmake_dir}"
  NO_DEFAULT_PATH
)

set(headers
  ArrayHandleToNumPy.h
  NumPyToArrayHandle.h
)

set(sources
  ArrayHandleToNumPy.cxx
  NumPyToArrayHandle.cxx
)

# Build as a static library so the object files are linked directly into the
# Python extension module.  This ensures viskores_interop_python and the
# extension share exactly one copy of the nanobind runtime (whatever variant
# nanobind_add_module picks, e.g. nanobind-static-abi3 for stable-ABI builds).
viskores_library(
  NAME viskores_interop_python
  STATIC
  SOURCES ${sources}
  HEADERS ${headers}
)

target_compile_definitions(viskores_interop_python PRIVATE PY_SSIZE_T_CLEAN)

# Provide nanobind and Python headers at compile time.  The nanobind runtime
# itself is supplied by the consumer (the extension module).
target_include_directories(viskores_interop_python
  PRIVATE ${NB_DIR}/include
  PRIVATE ${Python_INCLUDE_DIRS}
)
nanobind_compile_options(viskores_interop_python)
