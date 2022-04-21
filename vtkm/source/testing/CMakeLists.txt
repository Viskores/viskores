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
  UnitTestOscillatorSource.cxx
  UnitTestTangleSource.cxx
  UnitTestWaveletSource.cxx
 )

set(libraries
  vtkm_source
  )

if (VTKm_ENABLE_RENDERING)
  list(APPEND libraries vtkm_rendering_testing vtkm_filter_contour)

  list(APPEND unit_tests
    RenderTestPerlinNoise.cxx
    )
endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
)
