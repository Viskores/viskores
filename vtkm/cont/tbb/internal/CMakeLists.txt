##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  ArrayManagerExecutionTBB.h
  DeviceAdapterAlgorithmTBB.h
  DeviceAdapterRuntimeDetectorTBB.h
  DeviceAdapterTagTBB.h
  ExecutionArrayInterfaceBasicTBB.h
  FunctorsTBB.h
  ParallelSortTBB.h
  VirtualObjectTransferTBB.h
  )

if (VTKm_ENABLE_TBB)
  if("${TBB_VERSION_MAJOR}.${TBB_VERSION_MINOR}" VERSION_LESS 4.3)
    if("${TBB_VERSION_MAJOR}.${TBB_VERSION_MINOR}" VERSION_LESS 4.0)
      # MESSAGE(FATAL_ERROR "VTK-m not tested with TBB before version 4.0. Please upgrade your TBB library.")
    endif()
  endif()
endif()

vtkm_declare_headers(ParallelSortTBB.hxx
                     parallel_sort.h
                     TESTABLE OFF)

vtkm_declare_headers(${headers} TESTABLE ${VTKm_ENABLE_TBB})

#These sources need to always be built
target_sources(vtkm_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorTBB.cxx
  )

#-----------------------------------------------------------------------------
if (NOT VTKm_ENABLE_TBB)
  return()
endif()

target_sources(vtkm_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/ArrayManagerExecutionTBB.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmTBB.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/ExecutionArrayInterfaceBasicTBB.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/ParallelSortTBB.cxx
)
