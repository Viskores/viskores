##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(density_estimate_headers
  ContinuousScatterPlot.h
  Entropy.h
  Histogram.h
  NDEntropy.h
  NDHistogram.h
  ParticleDensityBase.h
  ParticleDensityCloudInCell.h
  ParticleDensityNearestGridPoint.h
  Statistics.h
  )

set(density_estimate_sources_device
  ContinuousScatterPlot.cxx
  Entropy.cxx
  Histogram.cxx
  NDEntropy.cxx
  NDHistogram.cxx
  ParticleDensityBase.cxx
  ParticleDensityCloudInCell.cxx
  ParticleDensityNearestGridPoint.cxx
  Statistics.cxx
  )

vtkm_library(
  NAME vtkm_filter_density_estimate
  HEADERS ${density_estimate_headers}
  DEVICE_SOURCES ${density_estimate_sources_device}
  USE_VTKM_JOB_POOL
)

set_property(TARGET
  vtkm_filter_density_estimate
  PROPERTY UNITY_BUILD_MODE GROUP
  )

target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_density_estimate)

add_subdirectory(worklet)
