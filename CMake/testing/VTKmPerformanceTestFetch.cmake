##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

include(${VTKm_SOURCE_DIR}/CMake/testing/VTKmPerformanceTestLib.cmake)

REQUIRE_FLAG_MUTABLE("VTKm_PERF_REPO")
REQUIRE_FLAG_MUTABLE("VTKm_PERF_REMOTE_URL")

file(REMOVE_RECURSE vtk-m-benchmark-records)
execute(COMMAND /usr/bin/git clone -b records ${VTKm_PERF_REMOTE_URL} ${VTKm_PERF_REPO})
