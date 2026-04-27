##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

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
  message(FATAL_ERROR "nanobind is required to build Viskores Python bindings.")
endif()

find_package(nanobind CONFIG REQUIRED
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

execute_process(
  COMMAND "${Python_EXECUTABLE}" -c "import numpy; print(numpy.get_include())"
  OUTPUT_VARIABLE Viskores_PYTHON_NUMPY_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE Viskores_PYTHON_NUMPY_RESULT
)
if (NOT Viskores_PYTHON_NUMPY_RESULT EQUAL 0)
  message(FATAL_ERROR "NumPy is required to build Viskores Python bindings.")
endif()

set(Viskores_PYTHON_PACKAGE_DIR "${Viskores_BINARY_DIR}/python_bindings/viskores")
file(MAKE_DIRECTORY "${Viskores_PYTHON_PACKAGE_DIR}")

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
  "${Viskores_PYTHON_PACKAGE_DIR}"
  "${VISKORES_PYTHON_STABLE_ABI}"
)

file(GLOB_RECURSE Viskores_PYTHON_PACKAGE_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/viskores/*.py"
)
set(Viskores_PYTHON_PACKAGE_STAMP "${Viskores_BINARY_DIR}/python_bindings/.python_package_sync.stamp")

add_custom_command(
  OUTPUT "${Viskores_PYTHON_PACKAGE_STAMP}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${Viskores_PYTHON_PACKAGE_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/viskores"
          "${Viskores_PYTHON_PACKAGE_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E remove_directory
          "${Viskores_PYTHON_PACKAGE_DIR}/filter/scalar_topology/highlevel"
  COMMAND "${CMAKE_COMMAND}" -E touch "${Viskores_PYTHON_PACKAGE_STAMP}"
  DEPENDS ${Viskores_PYTHON_PACKAGE_FILES}
)
add_custom_target(viskores_python_package_sync DEPENDS "${Viskores_PYTHON_PACKAGE_STAMP}")

nanobind_add_module(viskores_python_bindings ${Viskores_PYTHON_NANOBIND_STABLE_ABI} NOMINSIZE
  viskores_python_bindings.cxx
  src/common_support_bindings.cxx
  src/shared_data_bindings.cxx
  src/source_testing_io_bindings.cxx
  src/filter_basic_bindings.cxx
  src/filter_helper_bindings.cxx
  src/filter_field_transform_bindings.cxx
  src/filter_density_estimate_bindings.cxx
  src/filter_image_processing_bindings.cxx
  src/filter_additional_bindings.cxx
  src/filter_geometry_refinement_bindings.cxx
  src/filter_resampling_bindings.cxx
  src/scalar_topology_helper_bindings.cxx
  src/scalar_topology_bindings.cxx
  src/interop_support_bindings.cxx
  src/interop_helper_bindings.cxx
  src/rendering_support_bindings.cxx
  src/rendering_bindings.cxx
  src/module_bindings.cxx
)
target_compile_features(viskores_python_bindings PRIVATE cxx_std_17)
target_include_directories(viskores_python_bindings
  PRIVATE
    ${Viskores_PYTHON_NUMPY_INCLUDE_DIR}
)
target_link_libraries(viskores_python_bindings PRIVATE viskores_cont)
if (TARGET viskores_openmp)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_openmp)
elseif (TARGET viskores::openmp)
  target_link_libraries(viskores_python_bindings PRIVATE viskores::openmp)
endif()

set(Viskores_PYTHON_BINDING_DEFINITIONS)

if (TARGET viskores_source)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_source)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_SOURCE=1)
endif()

if (TARGET viskores_filter_core)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_core)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_CORE=1)
endif()

if (TARGET viskores_filter_contour)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_contour)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_CONTOUR=1)
endif()

if (TARGET viskores_filter_field_conversion)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_field_conversion)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION=1)
endif()

if (TARGET viskores_filter_field_transform)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_field_transform)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_FIELD_TRANSFORM=1)
endif()

if (TARGET viskores_filter_density_estimate)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_density_estimate)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_DENSITY_ESTIMATE=1)
endif()

if (TARGET viskores_filter_image_processing)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_image_processing)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_IMAGE_PROCESSING=1)
endif()

if (TARGET viskores_filter_clean_grid)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_clean_grid)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_CLEAN_GRID=1)
endif()

if (TARGET viskores_filter_scalar_topology)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_scalar_topology)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY=1)
endif()

if (TARGET viskores_filter_vector_analysis)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_vector_analysis)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS=1)
endif()

if (TARGET viskores_filter_connected_components)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_connected_components)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_CONNECTED_COMPONENTS=1)
endif()

if (TARGET viskores_filter_entity_extraction)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_entity_extraction)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION=1)
endif()

if (TARGET viskores_filter_mesh_info)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_mesh_info)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO=1)
endif()

if (TARGET viskores_filter_multi_block)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_multi_block)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_MULTI_BLOCK=1)
endif()

if (TARGET viskores_filter_geometry_refinement)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_geometry_refinement)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_GEOMETRY_REFINEMENT=1)
endif()

if (TARGET viskores_filter_resampling)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_filter_resampling)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_FILTER_RESAMPLING=1)
endif()

if (TARGET viskores_io)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_io)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_IO=1)
  if (Viskores_ENABLE_HDF5_IO)
    list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_IO_HDF5=1)
  endif()
endif()

if (TARGET viskores_interop)
  if (NOT TARGET OpenGL::GL)
    find_package(OpenGL REQUIRED)
  endif()
  target_link_libraries(viskores_python_bindings PRIVATE viskores_interop)
  if (TARGET OpenGL::GL)
    target_link_libraries(viskores_python_bindings PRIVATE OpenGL::GL)
  endif()
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_INTEROP=1)
endif()

if (TARGET viskores_rendering)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_rendering)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_RENDERING=1)
endif()

if (TARGET viskores_cont_testing)
  target_link_libraries(viskores_python_bindings PRIVATE viskores_cont_testing)
  list(APPEND Viskores_PYTHON_BINDING_DEFINITIONS VISKORES_PYTHON_ENABLE_TESTING_UTILS=1)
endif()

target_compile_definitions(viskores_python_bindings PRIVATE PY_SSIZE_T_CLEAN)
target_compile_definitions(viskores_python_bindings PRIVATE ${Viskores_PYTHON_BINDING_DEFINITIONS})
add_dependencies(viskores_python_bindings viskores_python_package_sync)
set_target_properties(viskores_python_bindings PROPERTIES
  PREFIX ""
  OUTPUT_NAME "_viskores"
  LIBRARY_OUTPUT_DIRECTORY "${Viskores_PYTHON_PACKAGE_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${Viskores_PYTHON_PACKAGE_DIR}"
)

if (Viskores_ENABLE_TESTING)
  function(viskores_add_python_binding_test test_name script_path)
    add_test(
      NAME ${test_name}
      COMMAND
        "${Python_EXECUTABLE}"
        "${script_path}"
    )
    set_tests_properties(${test_name} PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
    )
  endfunction()

  add_test(
    NAME PythonBindingsSmokeTest
    COMMAND
      "${Python_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/smoke_test.py"
  )
  set_tests_properties(PythonBindingsSmokeTest PROPERTIES
    ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
  )

  viskores_add_python_binding_test(
    UnitTestPythonWrapperDocstrings
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_docstrings.py"
  )

  viskores_add_python_binding_test(
    UnitTestPythonWrapperInitialize
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_initialize.py"
  )
  viskores_add_python_binding_test(
    UnitTestPythonWrapperRangeClass
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_range_class.py"
  )
  viskores_add_python_binding_test(
    UnitTestPythonWrapperFieldClass
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_field_class.py"
  )
  viskores_add_python_binding_test(
    UnitTestPythonWrapperNumPyInterop
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_numpy_interop.py"
  )
  viskores_add_python_binding_test(
    UnitTestPythonWrapperCoordinateSystemClass
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_coordinate_system_class.py"
  )
  viskores_add_python_binding_test(
    UnitTestPythonWrapperDataSetClass
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_dataset_class.py"
  )
  viskores_add_python_binding_test(
    UnitTestPythonWrapperDataSetBuilders
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_dataset_builders.py"
  )
  viskores_add_python_binding_test(
    UnitTestPythonWrapperCellSetClass
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_cell_set_class.py"
  )
  viskores_add_python_binding_test(
    UnitTestPythonWrapperPartitionedDataSetClass
    "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_partitioned_dataset_class.py"
  )

  if (TARGET viskores_source)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperTangleSource
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_tangle_source.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperOscillatorSource
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_oscillator_source.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperWaveletSource
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_wavelet_source.py"
    )
  endif()

  if (TARGET viskores_rendering)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperColorTableClass
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_color_table_class.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperCameraClass
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_camera_class.py"
    )
  endif()

  if (TARGET viskores_filter_contour AND TARGET viskores_source)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperContourFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_contour_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperContourFilterNormals
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_contour_filter_normals.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperClipWithFieldFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_clip_with_field_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperClipWithImplicitFunctionFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_clip_with_implicit_function_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperSliceMultipleFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_slice_multiple_filter.py"
    )
  endif()

  if (TARGET viskores_filter_contour AND TARGET viskores_io)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperMIRFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mir_filter.py"
    )
  endif()

  if (TARGET viskores_filter_vector_analysis)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperCrossProductFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_cross_product_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperDotProductFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_dot_product_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperVectorMagnitudeFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_vector_magnitude_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperGradientExplicit
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_gradient_explicit.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperGradientUniform
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_gradient_uniform.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperSurfaceNormalsFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_surface_normals_filter.py"
    )
  endif()

  if (TARGET viskores_filter_field_conversion)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperCellAverageFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_cell_average_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperPointAverageFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_point_average_filter.py"
    )
  endif()

  if (TARGET viskores_filter_field_transform)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperCompositeVectors
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_composite_vectors.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperCoordinateSystemTransform
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_coordinate_system_transform.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperFieldToColors
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_field_to_colors.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperGenerateIds
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_generate_ids.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperLogValues
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_log_values.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperPointElevationFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_point_elevation_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperPointTransform
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_point_transform.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperWarpFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_warp_filter.py"
    )
  endif()

  if (TARGET viskores_filter_density_estimate)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperContinuousScatterPlot
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_continuous_scatter_plot.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperEntropyFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_entropy_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperHistogramFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_histogram_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperNDEntropyFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_nd_entropy_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperNDHistogramFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_nd_histogram_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperPartitionedDataSetHistogramFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_partitioned_dataset_histogram_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperStatisticsFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_statistics_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperParticleDensity
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_particle_density.py"
    )
  endif()

  if (TARGET viskores_filter_image_processing)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperImageDifferenceFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_image_difference_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperImageMedianFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_image_median_filter.py"
    )
  endif()

  if (TARGET viskores_filter_scalar_topology AND TARGET viskores_filter_core)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperContourTreeUniformFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_contour_tree_uniform_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperContourTreeUniformDistributedFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_contour_tree_uniform_distributed_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperContourTreeUniformAugmentedFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_contour_tree_uniform_augmented_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperDistributedBranchDecompositionFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_distributed_branch_decomposition_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperScalarTopologyFilters
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_scalar_topology_filters.py"
    )
  endif()

  if (TARGET viskores_filter_resampling)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperProbeFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_probe_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperHistSampling
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_hist_sampling.py"
    )
  endif()

  if (TARGET viskores_io)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperBOVDataSetReader
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_bov_dataset_reader.py"
    )
    if (Viskores_ENABLE_HDF5_IO)
      viskores_add_python_binding_test(
        UnitTestPythonWrapperHDF5Image
        "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_hdf5_image.py"
      )
    endif()
    viskores_add_python_binding_test(
      UnitTestPythonWrapperImageWriter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_image_writer.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperVTKVisItFileReader
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_visit_file_reader.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperVTKDataSetReader
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_vtk_dataset_reader.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperVTKDataSetWriter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_vtk_dataset_writer.py"
    )
  endif()

  if (TARGET viskores_interop)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperInteropSurface
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_interop_surface.py"
    )
  endif()

  if (TARGET viskores_filter_entity_extraction)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperExternalFacesFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_external_faces_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperExtractGeometryFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_extract_geometry_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperExtractPointsFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_extract_points_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperExtractStructuredFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_extract_structured_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperGhostCellRemove
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_ghost_cell_remove.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperMaskFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mask_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperMaskPointsFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mask_points_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperThresholdPointsFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_threshold_points_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperThresholdFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_threshold_filter.py"
    )
  endif()

  if (TARGET viskores_filter_clean_grid AND TARGET viskores_filter_contour)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperCleanGrid
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_clean_grid.py"
    )
  endif()

  if (TARGET viskores_filter_geometry_refinement)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperConvertToPointCloud
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_convert_to_point_cloud.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperShrinkFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_shrink_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperSplitSharpEdgesFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_split_sharp_edges_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperTetrahedralizeFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_tetrahedralize_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperTubeFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_tube_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperTriangulateFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_triangulate_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperVertexClusteringFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_vertex_clustering_filter.py"
    )
  endif()

  if (TARGET viskores_filter_mesh_info)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperGhostCellClassify
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_ghost_cell_classify.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperCellMeasuresFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_cell_measures_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperMeshQualityFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mesh_quality_filter.py"
    )
  endif()

  if (TARGET viskores_filter_connected_components)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperCellSetConnectivityFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_cell_set_connectivity_filter.py"
    )
    viskores_add_python_binding_test(
      UnitTestPythonWrapperImageConnectivityFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_image_connectivity_filter.py"
    )
  endif()

  if (TARGET viskores_filter_multi_block)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperMergeDataSetsFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_merge_datasets_filter.py"
    )
  endif()

  if (TARGET viskores_rendering AND TARGET viskores_source AND TARGET viskores_filter_contour)
    viskores_add_python_binding_test(
      PythonWrapperRenderTestPerlinNoise
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_perlin_noise.py"
    )
    set_tests_properties(PythonWrapperRenderTestPerlinNoise PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      PythonWrapperRenderTestContourFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_contour_filter.py"
    )
    set_tests_properties(PythonWrapperRenderTestContourFilter PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )
  endif()

  if (TARGET viskores_rendering AND TARGET viskores_source AND TARGET viskores_filter_contour AND TARGET viskores_filter_geometry_refinement)
    viskores_add_python_binding_test(
      PythonWrapperRenderTestSliceFilter
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_slice_filter.py"
    )
    set_tests_properties(PythonWrapperRenderTestSliceFilter PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    if (TARGET viskores_io)
      viskores_add_python_binding_test(
        PythonWrapperRenderTestSplitSharpEdges
        "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_split_sharp_edges.py"
      )
      set_tests_properties(PythonWrapperRenderTestSplitSharpEdges PROPERTIES
        ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
        WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
      )
    endif()
  endif()

  if (TARGET viskores_rendering AND TARGET viskores_cont_testing)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperCanvas
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_canvas.py"
    )
    set_tests_properties(UnitTestPythonWrapperCanvas PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      UnitTestPythonWrapperMapperWireframer
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mapper_wireframer.py"
    )
    set_tests_properties(UnitTestPythonWrapperMapperWireframer PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      PythonWrapperExampleDemo
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_demo.py"
    )
    set_tests_properties(PythonWrapperExampleDemo PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      PythonWrapperExampleGameOfLife
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_game_of_life.py"
    )
    set_tests_properties(PythonWrapperExampleGameOfLife PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )
  endif()

  if (TARGET viskores_io AND TARGET viskores_filter_contour AND TARGET viskores_cont_testing)
    viskores_add_python_binding_test(
      PythonWrapperExampleClipping
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/example_test_clipping.py"
    )
  endif()

  if (TARGET viskores_io AND TARGET viskores_filter_geometry_refinement AND TARGET viskores_cont_testing)
    viskores_add_python_binding_test(
      PythonWrapperExampleTriangulate
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/example_test_triangulate.py"
    )
    viskores_add_python_binding_test(
      PythonWrapperExampleTetrahedralize
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/example_test_tetrahedralize.py"
    )
  endif()

  if (TARGET viskores_rendering AND TARGET viskores_filter_geometry_refinement)
    viskores_add_python_binding_test(
      PythonWrapperExamplePolyLineArchimedeanHelix
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/example_test_polyline_archimedean_helix.py"
    )
    set_tests_properties(PythonWrapperExamplePolyLineArchimedeanHelix PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )
  endif()

  if (TARGET viskores_rendering AND TARGET viskores_source)
    viskores_add_python_binding_test(
      PythonWrapperRenderTestAlternateCoordinates
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_alternate_coordinates.py"
    )
    set_tests_properties(PythonWrapperRenderTestAlternateCoordinates PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )
  endif()

  if (TARGET viskores_rendering AND TARGET viskores_source AND TARGET viskores_filter_image_processing)
    viskores_add_python_binding_test(
      PythonWrapperRenderTestComputeMoments
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_compute_moments.py"
    )
    set_tests_properties(PythonWrapperRenderTestComputeMoments PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )
  endif()

  if (TARGET viskores_rendering AND TARGET viskores_io AND TARGET viskores_filter_field_transform AND TARGET viskores_filter_vector_analysis)
    viskores_add_python_binding_test(
      PythonWrapperRenderTestPointTransform
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_point_transform.py"
    )
    set_tests_properties(PythonWrapperRenderTestPointTransform PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      PythonWrapperRenderTestSurfaceNormals
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/render_test_surface_normals.py"
    )
    set_tests_properties(PythonWrapperRenderTestSurfaceNormals PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )
  endif()

  if (TARGET viskores_rendering AND TARGET viskores_cont_testing)
    viskores_add_python_binding_test(
      UnitTestPythonWrapperMapperGlyphScalar
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mapper_glyph_scalar.py"
    )
    set_tests_properties(UnitTestPythonWrapperMapperGlyphScalar PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      UnitTestPythonWrapperMapperGlyphVector
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mapper_glyph_vector.py"
    )
    set_tests_properties(UnitTestPythonWrapperMapperGlyphVector PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      UnitTestPythonWrapperMapperCylinder
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mapper_cylinder.py"
    )
    set_tests_properties(UnitTestPythonWrapperMapperCylinder PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      UnitTestPythonWrapperMapperConnectivity
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mapper_connectivity.py"
    )
    set_tests_properties(UnitTestPythonWrapperMapperConnectivity PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      UnitTestPythonWrapperMapperPoints
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mapper_points.py"
    )
    set_tests_properties(UnitTestPythonWrapperMapperPoints PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      UnitTestPythonWrapperMapperQuads
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mapper_quads.py"
    )
    set_tests_properties(UnitTestPythonWrapperMapperQuads PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      UnitTestPythonWrapperMapperRayTracer
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mapper_raytracer.py"
    )
    set_tests_properties(UnitTestPythonWrapperMapperRayTracer PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      UnitTestPythonWrapperMapperVolume
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_mapper_volume.py"
    )
    set_tests_properties(UnitTestPythonWrapperMapperVolume PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )

    viskores_add_python_binding_test(
      UnitTestPythonWrapperScalarRenderer
      "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_test_scalar_renderer.py"
    )
    set_tests_properties(UnitTestPythonWrapperScalarRenderer PROPERTIES
      ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
      WORKING_DIRECTORY "${Viskores_BINARY_DIR}/python_bindings"
    )
  endif()
endif()
