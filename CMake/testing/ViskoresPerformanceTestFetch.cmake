##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

include(${Viskores_SOURCE_DIR}/CMake/testing/ViskoresPerformanceTestLib.cmake)

REQUIRE_FLAG("Viskores_SOURCE_DIR")
REQUIRE_FLAG_MUTABLE("Viskores_PERF_REPO")
REQUIRE_FLAG_MUTABLE("Viskores_PERF_REMOTE_URL")

set(upstream_url "https://github.com/Viskores/viskores.git")

file(REMOVE_RECURSE viskores-benchmark-records)
execute(COMMAND /usr/bin/git clone -b records ${Viskores_PERF_REMOTE_URL} ${Viskores_PERF_REPO})

# Fetch Viskores main git repo objects, this is needed to ensure that when running the CI
# from a fork project of Viskores it will have access to the latest git commits in
# the upstream viskores git repo.
execute(COMMAND /usr/bin/git -C ${Viskores_SOURCE_DIR} remote add upstream ${upstream_url})
execute(COMMAND /usr/bin/git -C ${Viskores_SOURCE_DIR} fetch upstream)
