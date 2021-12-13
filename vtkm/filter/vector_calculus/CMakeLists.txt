##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(vector_calculus_headers
  DotProduct.h)
set(vector_calculus_sources_device
  DotProduct.cxx)

vtkm_library(
  NAME vtkm_filter_vector_calculus
  HEADERS ${vector_calculus_headers}
  DEVICE_SOURCES ${vector_calculus_sources_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_vector_calculus PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_vector_calculus)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
