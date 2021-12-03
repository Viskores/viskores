##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(fieldtransform_headers
  GenerateIds.h)
set(fieldtransform_sources_device
  GenerateIds.cxx)

vtkm_library(
  NAME vtkm_filter_fieldtransform
  HEADERS ${fieldtransform_headers}
  DEVICE_SOURCES ${fieldtransform_sources_device}
  USE_VTKM_JOB_POOL
)

set_property(TARGET
  vtkm_filter_fieldtransform
  PROPERTY UNITY_BUILD_MODE GROUP
  )

target_link_libraries(vtkm_filter_fieldtransform PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_fieldtransform)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
