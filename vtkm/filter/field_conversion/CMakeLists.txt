##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(field_conversion_headers
  CellAverage.h
  PointAverage.h
  )
set(field_conversion_sources
  CellAverage.cxx
  PointAverage.cxx
  )

vtkm_library(
  NAME vtkm_filter_field_conversion
  HEADERS ${field_conversion_headers}
  DEVICE_SOURCES ${field_conversion_sources}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_field_conversion)

add_subdirectory(worklet)
