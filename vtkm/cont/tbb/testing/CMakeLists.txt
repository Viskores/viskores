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
  UnitTestTBBArrayHandle.cxx
  UnitTestTBBArrayHandleFancy.cxx
  UnitTestTBBArrayHandleVirtualCoordinates.cxx
  UnitTestTBBBitField.cxx
  UnitTestTBBCellLocatorRectilinearGrid.cxx
  UnitTestTBBCellLocatorUniformBins.cxx
  UnitTestTBBCellLocatorUniformGrid.cxx
  UnitTestTBBColorTable.cxx
  UnitTestTBBComputeRange.cxx
  UnitTestTBBDataSetExplicit.cxx
  UnitTestTBBDataSetSingleType.cxx
  UnitTestTBBDeviceAdapter.cxx
  UnitTestTBBImplicitFunction.cxx
  UnitTestTBBPointLocatorUniformGrid.cxx
  UnitTestTBBVirtualObjectHandle.cxx
  )

vtkm_unit_tests(SOURCES ${unit_tests} DEFINES VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)
