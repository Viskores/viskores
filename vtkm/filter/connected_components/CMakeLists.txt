##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(connected_components_headers
  CellSetConnectivity.h
  ImageConnectivity.h
  )
set(connected_components_sources_device
  CellSetConnectivity.cxx
  ImageConnectivity.cxx
  )

vtkm_library(
  NAME vtkm_filter_connected_components
  HEADERS ${connected_components_headers}
  DEVICE_SOURCES ${connected_components_sources_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_connected_components PRIVATE vtkm_worklet PUBLIC vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_connected_components)

add_subdirectory(worklet)
