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
  list(APPEND libraries vtkm_rendering_testing)

  list(APPEND unit_tests
    RenderTestPerlinNoise.cxx
    )

  # Currently to use the rendering in VTK-m, you have to compile with a device compiler.
  # We hope to change that in the near future, but until then we need to add the
  # `ALL_BACKENDS` flag to `vtkm_unit_tests` to compile with the device compiler. Here
  # is a hack to add it.
  list(APPEND unit_tests ALL_BACKENDS)
endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
)
