##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(unit_tests
  UnitTestCrossProductFilter.cxx
  UnitTestDotProductFilter.cxx
  UnitTestGradientExplicit.cxx
  UnitTestGradientUniform.cxx
  UnitTestSurfaceNormalsFilter.cxx
  UnitTestVectorMagnitudeFilter.cxx
  )

set(libraries
  viskores_filter_vector_analysis
  )

if (Viskores_ENABLE_RENDERING)
  list(APPEND libraries
    viskores_rendering_testing
    viskores_rendering)

  list(APPEND unit_tests
    RenderTestSurfaceNormals.cxx
    )
endif()

viskores_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VISKORES_JOB_POOL
)
