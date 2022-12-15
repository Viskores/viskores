##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(clean_grid_headers
  CleanGrid.h)
set(clean_grid_sources_device
  CleanGrid.cxx)

vtkm_library(
  NAME vtkm_filter_clean_grid
  HEADERS ${clean_grid_headers}
  DEVICE_SOURCES ${clean_grid_sources_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_clean_grid)

add_subdirectory(worklet)
