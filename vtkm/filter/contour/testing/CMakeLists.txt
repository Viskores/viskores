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
  UnitTestClipWithFieldFilter.cxx
  UnitTestClipWithImplicitFunctionFilter.cxx
  UnitTestContourFilterNormals.cxx
)

set(unit_tests_device
  UnitTestContourFilter.cxx # Algorithm used, needs device compiler
  UnitTestMIRFilter.cxx # Algorithm used, needs device compiler
  )

if (VTKm_ENABLE_RENDERING)
  list(APPEND unit_tests
    RenderTestContourFilter.cxx
    RenderTestSliceFilter.cxx
    )
endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  USE_VTKM_JOB_POOL
)
