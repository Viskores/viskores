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
  Tangle.cxx
  Wavelet.cxx
  PerlinNoise.cxx
  )

vtkm_library(NAME vtkm_source
  SOURCES ${sources}
  DEVICE_SOURCES ${device_sources}
  HEADERS ${headers}
  )
