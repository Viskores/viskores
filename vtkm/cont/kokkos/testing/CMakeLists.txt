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
  UnitTestKokkosArrayHandle.cxx
  UnitTestKokkosArrayHandleFancy.cxx
  UnitTestKokkosArrayHandleMultiplexer.cxx
  UnitTestKokkosBitField.cxx
  UnitTestKokkosCellLocatorRectilinearGrid.cxx
  UnitTestKokkosCellLocatorTwoLevel.cxx
  UnitTestKokkosCellLocatorUniformGrid.cxx
  UnitTestKokkosComputeRange.cxx
  UnitTestKokkosColorTable.cxx
  UnitTestKokkosDataSetExplicit.cxx
  UnitTestKokkosDataSetSingleType.cxx
  UnitTestKokkosDeviceAdapter.cxx
  UnitTestKokkosImplicitFunction.cxx
  UnitTestKokkosPointLocatorSparseGrid.cxx
  UnitTestKokkosRuntimeDeviceConfiguration.cxx
  )

if (NOT VTKm_NO_DEPRECATED_VIRTUAL)
  set(unit_tests ${unit_tests}
    UnitTestKokkosVirtualObjectHandle.cxx
    )
endif()

vtkm_unit_tests(SOURCES ${unit_tests} LABEL "KOKKOS" LIBRARIES vtkm_worklet)

if (TARGET vtkm::kokkos_cuda)
  set_source_files_properties(${unit_tests} PROPERTIES LANGUAGE CUDA)
elseif(TARGET vtkm::kokkos_hip)
  set_source_files_properties(${unit_tests} PROPERTIES LANGUAGE HIP)
endif()
