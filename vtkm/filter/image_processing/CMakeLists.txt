##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(image_processing_headers
  ComputeMoments.h
  ImageDifference.h
  ImageMedian.h
  )

set(image_processing_sources
  ComputeMoments.cxx
  ImageDifference.cxx
  ImageMedian.cxx
  )

vtkm_library(
  NAME vtkm_filter_image_processing
  HEADERS ${image_processing_headers}
  DEVICE_SOURCES ${image_processing_sources}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_image_processing)

add_subdirectory(worklet)
