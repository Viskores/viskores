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
  UnitTestTBBDeviceAdapter.cxx
  UnitTestTBBRuntimeDeviceConfiguration.cxx
  )

viskores_unit_tests(SOURCES ${unit_tests}
                LABEL "TBB"
                DEFINES VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG
                LIBRARIES viskores_worklet
                BACKEND tbb
                )
