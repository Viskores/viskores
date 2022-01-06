##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(entity_extraction_headers
  ExternalFaces.h
  ExtractPoints.h
  MaskPoints.h
  ThresholdPoints.h
  )
set(entity_extraction_sources_device
  ExternalFaces.cxx
  ExtractPoints.cxx
  MaskPoints.cxx
  ThresholdPoints.cxx
  )

vtkm_library(
  NAME vtkm_filter_entity_extraction
  HEADERS ${entity_extraction_headers}
  DEVICE_SOURCES ${entity_extraction_sources_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_entity_extraction PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_entity_extraction)

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
