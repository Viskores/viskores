##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(field_transform_headers
  CompositeVectors.h
  CylindricalCoordinateTransform.h
  FieldToColors.h
  GenerateIds.h
  LogValues.h
  PointElevation.h
  PointTransform.h
  SphericalCoordinateTransform.h
  WarpScalar.h
  WarpVector.h
  )

set(field_transform_sources
  CylindricalCoordinateTransform.cxx
  FieldToColors.cxx
  GenerateIds.cxx
  LogValues.cxx
  PointElevation.cxx
  PointTransform.cxx
  SphericalCoordinateTransform.cxx
  WarpScalar.cxx
  WarpVector.cxx
  )

set(field_transform_sources_no_device
  CompositeVectors.cxx
  )

vtkm_library(
  NAME vtkm_filter_field_transform
  HEADERS ${field_transform_headers}
  DEVICE_SOURCES ${field_transform_sources}
  SOURCES ${field_transform_sources_no_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_field_transform PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_field_transform)

add_subdirectory(worklet)
