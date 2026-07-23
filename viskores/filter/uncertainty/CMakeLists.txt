##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(uncertainty_headers
  ContourUncertainGaussianCorrelated.h
  ContourUncertainGaussianIndependent.h
  ContourUncertainUniform.h
  ContourUncertainUniformMonteCarlo.h
  FiberUncertainUniform.h
  )

set(uncertainty_sources_device
  ContourUncertainGaussianCorrelated.cxx
  ContourUncertainGaussianIndependent.cxx
  ContourUncertainUniform.cxx
  ContourUncertainUniformMonteCarlo.cxx
  FiberUncertainUniform.cxx
  )

viskores_library(
  NAME viskores_filter_uncertainty
  HEADERS ${uncertainty_headers}
  DEVICE_SOURCES ${uncertainty_sources_device}
  USE_VISKORES_JOB_POOL
)

add_subdirectory(worklet)
