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
  DeviceAdapterAlgorithmTBB.h
  DeviceAdapterMemoryManagerTBB.h
  DeviceAdapterRuntimeDetectorTBB.h
  DeviceAdapterTagTBB.h
  FunctorsTBB.h
  ParallelSortTBB.h
  RuntimeDeviceConfigurationTBB.h
  )

vtkm_declare_headers(${headers}
   ParallelSortTBB.hxx
   )

#These sources need to always be built
target_sources(vtkm_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorTBB.cxx
  )

#-----------------------------------------------------------------------------
if (TARGET vtkm_tbb)
  target_sources(vtkm_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmTBB.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ParallelSortTBB.cxx
    )
endif()
