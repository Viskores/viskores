##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

# Viskores' module system processes this file in an isolated scope, so the
# Python discovery from the parent python_bindings CMakeLists is not visible
# here. Re-run it locally; find_package hits the CMake cache, so this is cheap
# and does not depend on the parent's ordering.
find_package(Python REQUIRED COMPONENTS Interpreter)

add_test(
  NAME PythonBindingsUnknownArrayHandleNumPy
  COMMAND
    "${Python_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/PythonBindingsUnknownArrayHandleNumPy.py"
)
set_tests_properties(PythonBindingsUnknownArrayHandleNumPy PROPERTIES
  ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
)

add_test(
  NAME PythonBindingsDataSet
  COMMAND
    "${Python_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/PythonBindingsDataSet.py"
)
set_tests_properties(PythonBindingsDataSet PROPERTIES
  ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
)
