##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  DeviceAdapterAlgorithmOpenMP.h
  DeviceAdapterMemoryManagerOpenMP.h
  DeviceAdapterRuntimeDetectorOpenMP.h
  DeviceAdapterTagOpenMP.h
  FunctorsOpenMP.h
  ParallelQuickSortOpenMP.h
  ParallelRadixSortOpenMP.h
  ParallelScanOpenMP.h
  ParallelSortOpenMP.h
  RuntimeDeviceConfigurationOpenMP.h
  )

if (NOT VTKm_NO_DEPRECATED_VIRTUAL)
  set(headers ${headers}
    VirtualObjectTransferOpenMP.h
    )
endif()

vtkm_declare_headers(${headers})

#These sources need to always be built
target_sources(vtkm_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorOpenMP.cxx
  )

#-----------------------------------------------------------------------------
if (TARGET vtkm::openmp)
  target_sources(vtkm_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmOpenMP.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ParallelRadixSortOpenMP.cxx
    )
endif()
