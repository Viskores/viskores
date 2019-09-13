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
  UnitTestSerialArrayHandle.cxx
  UnitTestSerialArrayHandleFancy.cxx
  UnitTestSerialArrayHandleMultiplexer.cxx
  UnitTestSerialArrayHandleVirtualCoordinates.cxx
  UnitTestSerialBitField.cxx
  UnitTestSerialCellLocatorRectilinearGrid.cxx
  UnitTestSerialCellLocatorUniformBins.cxx
  UnitTestSerialCellLocatorUniformGrid.cxx
  UnitTestSerialComputeRange.cxx
  UnitTestSerialColorTable.cxx
  UnitTestSerialDataSetExplicit.cxx
  UnitTestSerialDataSetSingleType.cxx
  UnitTestSerialDeviceAdapter.cxx
  UnitTestSerialGeometry.cxx
  UnitTestSerialImplicitFunction.cxx
  UnitTestSerialPointLocatorUniformGrid.cxx
  UnitTestSerialVirtualObjectHandle.cxx
  )
vtkm_unit_tests(SOURCES ${unit_tests}
                LABEL "SERIAL"
                DEFINES VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG
                LIBRARIES vtkm_worklet)
