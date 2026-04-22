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

add_library(_viskores MODULE ../viskores_module.cxx)
target_compile_features(_viskores PRIVATE cxx_std_17)
target_compile_definitions(_viskores PRIVATE PY_SSIZE_T_CLEAN)
target_include_directories(_viskores
  PRIVATE
    ${Viskores_PYTHON_NUMPY_INCLUDE_DIR}
)
target_link_libraries(_viskores
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
set_target_properties(_viskores PROPERTIES
  PREFIX ""
  OUTPUT_NAME "${Viskores_PYTHON_EXTENSION_BASENAME}"
  SUFFIX "${Viskores_PYTHON_EXTENSION_SUFFIX}"
  LIBRARY_OUTPUT_DIRECTORY "${VISKORES_PYTHON_EXTENSION_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${VISKORES_PYTHON_EXTENSION_OUTPUT_DIR}"
)
