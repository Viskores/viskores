##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(vector_analysis_headers
  CrossProduct.h
  DotProduct.h
  Gradient.h
  SurfaceNormals.h
  VectorMagnitude.h
  )
set(vector_analysis_sources_device
  CrossProduct.cxx
  DotProduct.cxx
  Gradient.cxx
  SurfaceNormals.cxx
  VectorMagnitude.cxx
  )

vtkm_add_instantiations(instantiation_files
  INSTANTIATIONS_FILE worklet/Gradient.h
  )
list(APPEND vector_analysis_sources_device ${instantiation_files})

vtkm_library(
  NAME vtkm_filter_vector_analysis
  HEADERS ${vector_analysis_headers}
  DEVICE_SOURCES ${vector_analysis_sources_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_vector_analysis)

add_subdirectory(worklet)
