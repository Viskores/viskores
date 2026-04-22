##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(ViskoresPythonWheel LANGUAGES CXX)

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
find_package(Viskores REQUIRED CONFIG)
if (Viskores_ENABLE_OPENMP)
  find_package(OpenMP REQUIRED COMPONENTS CXX)
endif()

execute_process(
  COMMAND "${Python_EXECUTABLE}" -c "import numpy; print(numpy.get_include())"
  OUTPUT_VARIABLE Viskores_PYTHON_NUMPY_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE Viskores_PYTHON_NUMPY_RESULT
)
if (NOT Viskores_PYTHON_NUMPY_RESULT EQUAL 0)
  message(FATAL_ERROR "NumPy is required to build the Viskores Python wheel.")
endif()

if (NOT DEFINED VISKORES_PYTHON_EXTENSION_OUTPUT_DIR)
  message(FATAL_ERROR "VISKORES_PYTHON_EXTENSION_OUTPUT_DIR must be set.")
endif()
if (NOT DEFINED VISKORES_PYTHON_EXTENSION_FILENAME)
  message(FATAL_ERROR "VISKORES_PYTHON_EXTENSION_FILENAME must be set.")
endif()

get_filename_component(Viskores_PYTHON_EXTENSION_BASENAME
  "${VISKORES_PYTHON_EXTENSION_FILENAME}" NAME_WE)
get_filename_component(Viskores_PYTHON_EXTENSION_SUFFIX
  "${VISKORES_PYTHON_EXTENSION_FILENAME}" LAST_EXT)

add_library(viskores_python_bindings MODULE ../viskores_python_bindings.cxx)
target_compile_features(viskores_python_bindings PRIVATE cxx_std_17)
target_compile_definitions(viskores_python_bindings PRIVATE PY_SSIZE_T_CLEAN)
target_include_directories(viskores_python_bindings
  PRIVATE
    ${Viskores_PYTHON_NUMPY_INCLUDE_DIR}
)
target_link_libraries(viskores_python_bindings
  PRIVATE
    Python::Module
    $<$<BOOL:${Viskores_ENABLE_OPENMP}>:OpenMP::OpenMP_CXX>
    viskores::cont
    viskores::source
    viskores::filter_core
    viskores::filter_contour
    viskores::filter_field_conversion
    viskores::filter_scalar_topology
    viskores::filter_vector_analysis
    viskores::rendering
)
set_target_properties(viskores_python_bindings PROPERTIES
  PREFIX ""
  OUTPUT_NAME "${Viskores_PYTHON_EXTENSION_BASENAME}"
  SUFFIX "${Viskores_PYTHON_EXTENSION_SUFFIX}"
  LIBRARY_OUTPUT_DIRECTORY "${VISKORES_PYTHON_EXTENSION_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${VISKORES_PYTHON_EXTENSION_OUTPUT_DIR}"
)
