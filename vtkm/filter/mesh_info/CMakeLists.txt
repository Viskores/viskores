##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(mesh_info_headers
  CellMeasures.h
  GhostCellClassify.h
  MeshQuality.h
  )

set(mesh_info_sources
  CellMeasures.cxx
  GhostCellClassify.cxx
  MeshQuality.cxx
  )

vtkm_library(
  NAME vtkm_filter_mesh_info
  HEADERS ${mesh_info_headers}
  DEVICE_SOURCES ${mesh_info_sources}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_mesh_info PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_mesh_info)

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
