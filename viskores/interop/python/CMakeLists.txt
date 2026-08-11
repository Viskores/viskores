##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

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
