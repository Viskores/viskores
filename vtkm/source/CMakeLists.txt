##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  Amr.h
  Oscillator.h
  Source.h
  Tangle.h
  Wavelet.h
  PerlinNoise.h
  )

set(device_sources
  Amr.cxx
  Oscillator.cxx
  Source.cxx
  Tangle.cxx
  Wavelet.cxx
  PerlinNoise.cxx
  )

vtkm_library(NAME vtkm_source
  SOURCES ${sources}
  DEVICE_SOURCES ${device_sources}
  HEADERS ${headers}
  )

target_link_libraries(vtkm_source PUBLIC vtkm_cont vtkm_filter_field_conversion)

#-----------------------------------------------------------------------------
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
