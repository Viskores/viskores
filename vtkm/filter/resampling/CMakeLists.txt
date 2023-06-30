##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(resampling_headers
  Probe.h
  HistSampling.h
  )

set(resampling_sources
  Probe.cxx
  HistSampling.cxx
  )

vtkm_library(
  NAME vtkm_filter_resampling
  HEADERS ${resampling_headers}
  DEVICE_SOURCES ${resampling_sources}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_resampling)

add_subdirectory(worklet)
