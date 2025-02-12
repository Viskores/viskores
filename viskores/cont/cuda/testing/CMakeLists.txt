##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(unit_tests
  UnitTestCudaDeviceAdapter.cu
  UnitTestCudaIterators.cu
  UnitTestCudaMathEdgeCases.cu
  UnitTestCudaShareUserProvidedManagedMemory.cu
  UnitTestCudaRuntimeDeviceConfiguration.cu
  )

viskores_unit_tests(SOURCES ${unit_tests} LABEL "CUDA" LIBRARIES viskores_worklet BACKEND cuda)
