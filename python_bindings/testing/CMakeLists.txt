##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

add_test(
  NAME PythonBindingsUnknownArrayHandleNumPy
  COMMAND
    "${Python_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_unknown_array_numpy_roundtrip.py"
)
set_tests_properties(PythonBindingsUnknownArrayHandleNumPy PROPERTIES
  ENVIRONMENT "PYTHONPATH=${Viskores_BINARY_DIR}/python_bindings"
)
