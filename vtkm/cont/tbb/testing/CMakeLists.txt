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
  UnitTestTBBArrayHandleMultiplexer.cxx
  UnitTestTBBBitField.cxx
  UnitTestTBBCellLocatorRectilinearGrid.cxx
  UnitTestTBBCellLocatorTwoLevel.cxx
  UnitTestTBBCellLocatorUniformGrid.cxx
  UnitTestTBBColorTable.cxx
  UnitTestTBBComputeRange.cxx
  UnitTestTBBDataSetExplicit.cxx
  UnitTestTBBDataSetSingleType.cxx
  UnitTestTBBDeviceAdapter.cxx
  UnitTestTBBImplicitFunction.cxx
  UnitTestTBBPointLocatorSparseGrid.cxx
  )

if (NOT VTKm_NO_DEPRECATED_VIRTUAL)
  set(unit_tests ${unit_tests}
    UnitTestTBBVirtualObjectHandle.cxx
    )
endif()

vtkm_unit_tests(SOURCES ${unit_tests}
                LABEL "TBB"
                DEFINES VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG
                LIBRARIES vtkm_worklet)
