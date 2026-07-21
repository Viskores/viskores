##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(unit_tests
  UnitTestCudaDeviceAdapter.cu
  UnitTestCudaIterators.cu
  UnitTestCudaMathEdgeCases.cu
  UnitTestCudaShareUserProvidedManagedMemory.cu
  UnitTestCudaRuntimeDeviceConfiguration.cu
  )

viskores_unit_tests(
  LABEL "CUDA"
  LIBRARIES viskores_worklet
  BACKEND cuda
  DEVICE_SOURCES ${unit_tests}
  )
