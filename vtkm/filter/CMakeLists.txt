##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(core_headers
  FieldSelection.h
  NewFilter.h
  NewFilterField.h
  MapFieldMergeAverage.h
  MapFieldPermutation.h
  TaskQueue.h
  )
set(core_sources
  FieldSelection.cxx
  NewFilterField.cxx
  )
set(core_sources_device
  MapFieldMergeAverage.cxx
  MapFieldPermutation.cxx
  NewFilter.cxx
  )

vtkm_library(
  NAME vtkm_filter_core
  HEADERS ${core_headers}
  SOURCES ${core_sources}
  DEVICE_SOURCES ${core_sources_device}
  USE_VTKM_JOB_POOL
)

add_library(vtkm_filter INTERFACE)

set_target_properties(
  vtkm_filter_core
  PROPERTIES
  UNITY_BUILD ON
  UNITY_BUILD_MODE BATCH
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE
  vtkm_filter_core
  )

install(TARGETS vtkm_filter EXPORT ${VTKm_EXPORT_NAME})
