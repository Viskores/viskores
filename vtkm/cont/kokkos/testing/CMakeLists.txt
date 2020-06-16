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
  UnitTestKokkosAlgorithms.cxx
  UnitTestKokkosArrayHandle.cxx
  UnitTestKokkosArrayHandleFancy.cxx
  UnitTestKokkosArrayHandleMultiplexer.cxx
  UnitTestKokkosArrayHandleVirtualCoordinates.cxx
  UnitTestKokkosBitField.cxx
  UnitTestKokkosCellLocatorRectilinearGrid.cxx
  UnitTestKokkosCellLocatorUniformBins.cxx
  UnitTestKokkosCellLocatorUniformGrid.cxx
  UnitTestKokkosComputeRange.cxx
  UnitTestKokkosColorTable.cxx
  UnitTestKokkosDataSetExplicit.cxx
  UnitTestKokkosDataSetSingleType.cxx
  UnitTestKokkosDeviceAdapter.cxx
  UnitTestKokkosGeometry.cxx
  UnitTestKokkosImplicitFunction.cxx
  UnitTestKokkosPointLocatorUniformGrid.cxx
  UnitTestKokkosVirtualObjectHandle.cxx
  )
vtkm_unit_tests(SOURCES ${unit_tests} LABEL "KOKKOS" LIBRARIES vtkm_worklet)

if (TARGET vtkm::kokkos_cuda)
  set_source_files_properties(${unit_tests} PROPERTIES LANGUAGE CUDA)
endif()
