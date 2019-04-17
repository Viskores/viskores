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
  UnitTestCudaArrayHandle.cu
  UnitTestCudaArrayHandleFancy.cu
  UnitTestCudaArrayHandleVirtualCoordinates.cu
  UnitTestCudaBitField.cu
  UnitTestCudaCellLocatorRectilinearGrid.cu
  UnitTestCudaCellLocatorUniformBins.cu
  UnitTestCudaCellLocatorUniformGrid.cu
  UnitTestCudaComputeRange.cu
  UnitTestCudaColorTable.cu
  UnitTestCudaDataSetExplicit.cu
  UnitTestCudaDataSetSingleType.cu
  UnitTestCudaDeviceAdapter.cu
  UnitTestCudaGeometry.cu
  UnitTestCudaImplicitFunction.cu
  UnitTestCudaMath.cu
  UnitTestCudaShareUserProvidedManagedMemory.cu
  UnitTestCudaPointLocatorUniformGrid.cu
  UnitTestCudaVirtualObjectHandle.cu
  )
vtkm_unit_tests(SOURCES ${unit_tests})
