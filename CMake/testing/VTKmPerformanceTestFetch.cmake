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

REQUIRE_FLAG("VTKm_SOURCE_DIR")
REQUIRE_FLAG_MUTABLE("VTKm_PERF_REPO")
REQUIRE_FLAG_MUTABLE("VTKm_PERF_REMOTE_URL")

set(upstream_url "https://gitlab.kitware.com/vtk/vtk-m.git")

file(REMOVE_RECURSE vtk-m-benchmark-records)
execute(COMMAND /usr/bin/git clone -b records ${VTKm_PERF_REMOTE_URL} ${VTKm_PERF_REPO})

# Fetch VTK-m main git repo objects, this is needed to ensure that when running the CI
# from a fork project of VTK-m it will have access to the latest git commits in
# the upstream vtk-m git repo.
execute(COMMAND /usr/bin/git -C ${VTKm_SOURCE_DIR} remote add upstream ${upstream_url})
execute(COMMAND /usr/bin/git -C ${VTKm_SOURCE_DIR} fetch upstream)
