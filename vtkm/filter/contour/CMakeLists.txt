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
  Slice.h
)

set(contour_sources_device
  ClipWithField.cxx
  ClipWithImplicitFunction.cxx
  Contour.cxx
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

target_link_libraries(vtkm_filter_contour PRIVATE vtkm_worklet PUBLIC vtkm_filter_core vtkm_filter_vector_analysis)

if (VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_filter_contour PUBLIC MPI::MPI_CXX)
endif ()

target_link_libraries(vtkm_filter PUBLIC INTERFACE
   vtkm_filter_contour
)

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
