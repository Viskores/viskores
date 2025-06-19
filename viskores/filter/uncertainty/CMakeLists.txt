##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

viskores_get_kit_name(kit_name kit_dir)

set(uncertainty_headers
  ContourUncertainUniform.h
  ContourUncertainUniformMonteCarlo.h
  FiberUncertainUniform.h
  worklet/FiberUncertainUniform.h
  )

set(uncertainty_sources_device
  ContourUncertainUniform.cxx
  ContourUncertainUniformMonteCarlo.cxx
  FiberUncertainUniform.cxx
  )

viskores_declare_headers(${uncertainty_headers})

viskores_library(
  NAME viskores_filter_uncertainty
  HEADERS ${uncertainty_headers}
  DEVICE_SOURCES ${uncertainty_sources_device}
  USE_VISKORES_JOB_POOL
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/worklet
  DESTINATION ${Viskores_INSTALL_INCLUDE_DIR}/viskores/filter/uncertainty
  FILES_MATCHING PATTERN "*.h"
)
