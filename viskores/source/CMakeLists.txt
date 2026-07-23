##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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

viskores_library(NAME viskores_source
  SOURCES ${sources}
  DEVICE_SOURCES ${device_sources}
  HEADERS ${headers}
  )
