##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

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

add_library(_viskores MODULE viskores_module.cxx)
target_include_directories(_viskores
  PRIVATE
    ${Viskores_PYTHON_NUMPY_INCLUDE_DIR}
)
target_link_libraries(_viskores
  PRIVATE
    Python::Module
    viskores_cont
    viskores_source
    viskores_filter_core
    viskores_filter_contour
    viskores_filter_field_conversion
    viskores_filter_scalar_topology
    viskores_filter_vector_analysis
    viskores_rendering
)
target_compile_definitions(_viskores PRIVATE PY_SSIZE_T_CLEAN)
set_target_properties(_viskores PROPERTIES
  PREFIX ""
  OUTPUT_NAME "_viskores"
  LIBRARY_OUTPUT_DIRECTORY "${Viskores_PYTHON_PACKAGE_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${Viskores_PYTHON_PACKAGE_DIR}"
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/viskores/__init__.py"
  "${Viskores_PYTHON_PACKAGE_DIR}/__init__.py"
  COPYONLY
)

if (Viskores_ENABLE_TESTING)
  add_test(
    NAME PythonBindingsSmokeTest
    COMMAND
      "${Python_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_test.py"
  )
  set_tests_properties(PythonBindingsSmokeTest PROPERTIES
    ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
  )
endif()
