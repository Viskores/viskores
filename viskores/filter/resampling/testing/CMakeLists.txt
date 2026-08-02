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
set(unit_tests
  UnitTestProbe.cxx
  )
set(unit_tests_device
  UnitTestHistSampling.cxx
  )
set(testing_headers
  TestingHistSampling.h
  )

set_source_files_properties(${testing_headers} PROPERTIES NOT_A_TEST TRUE)

set(libraries
  viskores_filter_resampling
  )

viskores_unit_tests(
  SOURCES ${unit_tests} ${testing_headers}
  DEVICE_SOURCES ${unit_tests_device}
  LIBRARIES ${libraries}
  USE_VISKORES_JOB_POOL
)

if (Viskores_ENABLE_MPI)
  viskores_unit_tests(
    MPI
    SOURCES ${testing_headers}
    DEVICE_SOURCES UnitTestHistSamplingMPI.cxx
    LIBRARIES ${libraries}
    USE_VISKORES_JOB_POOL
  )
endif()
