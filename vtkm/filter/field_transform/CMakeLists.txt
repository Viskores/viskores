##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(fieldt_ransform_headers
  GenerateIds.h)
set(field_transform_sources_device
  GenerateIds.cxx)

vtkm_library(
  NAME vtkm_filter_field_transform
  HEADERS ${field_transform_headers}
  DEVICE_SOURCES ${field_transform_sources_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_field_transform PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_field_transform)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
