##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2018 UT-Battelle, LLC.
##  Copyright 2018 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  ArrayManagerExecutionOpenMP.h
  DeviceAdapterAlgorithmOpenMP.h
  DeviceAdapterRuntimeDetectorOpenMP.h
  DeviceAdapterTagOpenMP.h
  ExecutionArrayInterfaceBasicOpenMP.h
  FunctorsOpenMP.h
  ParallelQuickSortOpenMP.h
  ParallelRadixSortOpenMP.h
  ParallelScanOpenMP.h
  ParallelSortOpenMP.h
  VirtualObjectTransferOpenMP.h
  )

vtkm_declare_headers(${headers} TESTABLE ${VTKm_ENABLE_OPENMP})

target_sources(vtkm_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorOpenMP.cxx
  )

#-----------------------------------------------------------------------------
if (NOT VTKm_ENABLE_OPENMP)
  return()
endif()

target_sources(vtkm_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/ArrayManagerExecutionOpenMP.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmOpenMP.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/ExecutionArrayInterfaceBasicOpenMP.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/ParallelRadixSortOpenMP.cxx
)
