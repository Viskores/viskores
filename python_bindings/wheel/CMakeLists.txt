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

option(VISKORES_PYTHON_STABLE_ABI "Build Python bindings with the CPython stable ABI" OFF)

set(Viskores_PYTHON_COMPONENTS Interpreter Development)
if (VISKORES_PYTHON_STABLE_ABI AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
  list(APPEND Viskores_PYTHON_COMPONENTS Development.SABIModule)
endif()
find_package(Python REQUIRED COMPONENTS ${Viskores_PYTHON_COMPONENTS})

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_VARIABLE Viskores_PYTHON_NANOBIND_CMAKE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE Viskores_PYTHON_NANOBIND_RESULT
)
if (NOT Viskores_PYTHON_NANOBIND_RESULT EQUAL 0)
  message(FATAL_ERROR "nanobind is required to build the Viskores Python wheel.")
endif()

find_package(nanobind CONFIG REQUIRED
  PATHS "${Viskores_PYTHON_NANOBIND_CMAKE_DIR}"
  NO_DEFAULT_PATH
)
find_package(Viskores REQUIRED CONFIG)

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

if (Viskores_ENABLE_OPENMP)
  find_package(OpenMP REQUIRED COMPONENTS CXX)
endif()
if (Viskores_ENABLE_RENDERING)
  find_package(OpenGL REQUIRED)
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

function(viskores_python_remove_inactive_extension_files output_dir stable_abi)
  file(GLOB inactive_extension_candidates
    "${output_dir}/_viskores.*${CMAKE_SHARED_MODULE_SUFFIX}"
    "${output_dir}/_viskores.*${CMAKE_SHARED_LIBRARY_SUFFIX}"
  )
  foreach(inactive_extension IN LISTS inactive_extension_candidates)
    get_filename_component(inactive_extension_name "${inactive_extension}" NAME)
    if (stable_abi)
      if (NOT inactive_extension_name MATCHES "\\.abi3\\.")
        file(REMOVE "${inactive_extension}")
      endif()
    elseif (inactive_extension_name MATCHES "\\.abi3\\.")
      file(REMOVE "${inactive_extension}")
    endif()
  endforeach()
endfunction()
viskores_python_remove_inactive_extension_files(
  "${VISKORES_PYTHON_EXTENSION_OUTPUT_DIR}"
  "${VISKORES_PYTHON_STABLE_ABI}"
)

nanobind_add_module(viskores_python_bindings ${Viskores_PYTHON_NANOBIND_STABLE_ABI} NOMINSIZE
  ../viskores_python_bindings.cxx
  ../src/common_support_bindings.cxx
  ../src/shared_data_bindings.cxx
  ../src/source_testing_io_bindings.cxx
  ../src/filter_basic_bindings.cxx
  ../src/filter_helper_bindings.cxx
  ../src/filter_field_transform_bindings.cxx
  ../src/filter_density_estimate_bindings.cxx
  ../src/filter_image_processing_bindings.cxx
  ../src/filter_additional_bindings.cxx
  ../src/filter_geometry_refinement_bindings.cxx
  ../src/filter_resampling_bindings.cxx
  ../src/scalar_topology_helper_bindings.cxx
  ../src/scalar_topology_bindings.cxx
  ../src/interop_support_bindings.cxx
  ../src/interop_helper_bindings.cxx
  ../src/rendering_support_bindings.cxx
  ../src/rendering_bindings.cxx
  ../src/module_bindings.cxx
)
target_compile_features(viskores_python_bindings PRIVATE cxx_std_17)
target_compile_definitions(viskores_python_bindings PRIVATE PY_SSIZE_T_CLEAN)
target_include_directories(viskores_python_bindings
  PRIVATE
    ${Viskores_PYTHON_NUMPY_INCLUDE_DIR}
)
target_link_libraries(viskores_python_bindings PRIVATE viskores::cont)

set(Viskores_PYTHON_BINDING_DEFINITIONS)

if (TARGET viskores::source)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::source)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_SOURCE=1)
endif()

if (TARGET viskores::filter_core)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_core)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_CORE=1)
endif()

if (TARGET viskores::filter_contour)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_contour)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_CONTOUR=1)
endif()

if (TARGET viskores::filter_field_conversion)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_field_conversion)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION=1)
endif()

if (TARGET viskores::filter_field_transform)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_field_transform)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_FIELD_TRANSFORM=1)
endif()

if (TARGET viskores::filter_density_estimate)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_density_estimate)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_DENSITY_ESTIMATE=1)
endif()

if (TARGET viskores::filter_image_processing)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_image_processing)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_IMAGE_PROCESSING=1)
endif()

if (TARGET viskores::filter_clean_grid)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_clean_grid)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_CLEAN_GRID=1)
endif()

if (TARGET viskores::filter_scalar_topology)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_scalar_topology)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY=1)
endif()

if (TARGET viskores::filter_vector_analysis)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_vector_analysis)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS=1)
endif()

if (TARGET viskores::filter_connected_components)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_connected_components)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_CONNECTED_COMPONENTS=1)
endif()

if (TARGET viskores::filter_entity_extraction)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_entity_extraction)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION=1)
endif()

if (TARGET viskores::filter_mesh_info)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_mesh_info)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO=1)
endif()

if (TARGET viskores::filter_multi_block)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_multi_block)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_MULTI_BLOCK=1)
endif()

if (TARGET viskores::filter_geometry_refinement)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_geometry_refinement)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_GEOMETRY_REFINEMENT=1)
endif()

if (TARGET viskores::filter_resampling)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::filter_resampling)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_RESAMPLING=1)
endif()

if (TARGET viskores::io)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::io)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_IO=1)
  if (Viskores_ENABLE_HDF5_IO)
    list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_IO_HDF5=1)
  endif()
endif()

if (TARGET viskores::interop)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::interop)
  if (TARGET OpenGL::GL)
    target_link_libraries(viskores_python_bindings PRIVATE OpenGL::GL)
  endif()
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_INTEROP=1)
endif()

if (TARGET viskores::rendering)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::rendering)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_RENDERING=1)
endif()

if (TARGET viskores::cont_testing)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::cont_testing)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_TESTING_UTILS=1)
endif()

if (Viskores_ENABLE_OPENMP)
  target_link_libraries(viskores_python_bindings PRIVATE OpenMP::OpenMP_CXX)
endif()

target_compile_definitions(viskores_python_bindings PRIVATE ${Viskores_PYTHON_BINDING_DEFINITIONS})
set_target_properties(viskores_python_bindings PROPERTIES
  PREFIX ""
  OUTPUT_NAME "${Viskores_PYTHON_EXTENSION_BASENAME}"
  SUFFIX "${Viskores_PYTHON_EXTENSION_SUFFIX}"
  LIBRARY_OUTPUT_DIRECTORY "${VISKORES_PYTHON_EXTENSION_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${VISKORES_PYTHON_EXTENSION_OUTPUT_DIR}"
)
