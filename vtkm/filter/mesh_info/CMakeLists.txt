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
  MeshQualityArea.h
  MeshQualityAspectGamma.h
  MeshQualityAspectRatio.h
  MeshQualityCondition.h
  MeshQualityDiagonalRatio.h
  MeshQualityDimension.h
  MeshQualityJacobian.h
  MeshQualityMaxAngle.h
  MeshQualityMaxDiagonal.h
  MeshQualityMinAngle.h
  MeshQualityMinDiagonal.h
  MeshQualityOddy.h
  MeshQualityRelativeSizeSquared.h
  MeshQualityScaledJacobian.h
  MeshQualityShape.h
  MeshQualityShapeAndSize.h
  MeshQualityShear.h
  MeshQualitySkew.h
  MeshQualityStretch.h
  MeshQualityTaper.h
  MeshQualityVolume.h
  MeshQualityWarpage.h
  )

set(mesh_info_sources
  CellMeasures.cxx
  GhostCellClassify.cxx
  MeshQuality.cxx
  MeshQualityArea.cxx
  MeshQualityAspectGamma.cxx
  MeshQualityAspectRatio.cxx
  MeshQualityCondition.cxx
  MeshQualityDiagonalRatio.cxx
  MeshQualityDimension.cxx
  MeshQualityJacobian.cxx
  MeshQualityMaxAngle.cxx
  MeshQualityMaxDiagonal.cxx
  MeshQualityMinAngle.cxx
  MeshQualityMinDiagonal.cxx
  MeshQualityOddy.cxx
  MeshQualityRelativeSizeSquared.cxx
  MeshQualityScaledJacobian.cxx
  MeshQualityShape.cxx
  MeshQualityShapeAndSize.cxx
  MeshQualityShear.cxx
  MeshQualitySkew.cxx
  MeshQualityStretch.cxx
  MeshQualityTaper.cxx
  MeshQualityVolume.cxx
  MeshQualityWarpage.cxx
  )

vtkm_library(
  NAME vtkm_filter_mesh_info
  HEADERS ${mesh_info_headers}
  DEVICE_SOURCES ${mesh_info_sources}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_mesh_info)

add_subdirectory(worklet)
