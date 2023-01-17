##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(geometry_refinement_headers
  SplitSharpEdges.h
  Tetrahedralize.h
  Triangulate.h
  Tube.h
  VertexClustering.h
  )

set(geometry_refinement_sources
  SplitSharpEdges.cxx
  Tetrahedralize.cxx
  Triangulate.cxx
  Tube.cxx
  VertexClustering.cxx
  )

vtkm_library(
  NAME vtkm_filter_geometry_refinement
  HEADERS ${geometry_refinement_headers}
  DEVICE_SOURCES ${geometry_refinement_sources}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_geometry_refinement PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_geometry_refinement)

add_subdirectory(worklet)
