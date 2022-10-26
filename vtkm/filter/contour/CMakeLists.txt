##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(contour_headers
  ClipWithField.h
  ClipWithImplicitFunction.h
  Contour.h
  MIRFilter.h
  Slice.h
)

set(contour_sources_device
  ClipWithField.cxx
  ClipWithImplicitFunction.cxx
  Contour.cxx
  MIRFilter.cxx
  Slice.cxx
)

set_source_files_properties(Contour.cxx PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

vtkm_library(
  NAME vtkm_filter_contour
  HEADERS ${contour_headers}
  DEVICE_SOURCES ${contour_sources_device}
  USE_VTKM_JOB_POOL
)

set_property(TARGET
  vtkm_filter_contour
  PROPERTY UNITY_BUILD_MODE GROUP
)

if (VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_filter_contour PUBLIC MPI::MPI_CXX)
endif ()

target_link_libraries(vtkm_filter PUBLIC INTERFACE
   vtkm_filter_contour
)

add_subdirectory(worklet)
