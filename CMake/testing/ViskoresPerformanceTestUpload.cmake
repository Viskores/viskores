##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

include("${Viskores_SOURCE_DIR}/CMake/testing/ViskoresPerformanceTestLib.cmake")

REQUIRE_FLAG("Viskores_PERF_COMPARE_JSON")
REQUIRE_FLAG_MUTABLE("Viskores_PERF_REPO")

file(COPY "${Viskores_PERF_COMPARE_JSON}" DESTINATION "${Viskores_PERF_REPO}/")
get_filename_component(perf_report_name "${Viskores_PERF_COMPARE_JSON}" NAME)

execute(COMMAND /usr/bin/git -C "${Viskores_PERF_REPO}" config --local user.name viskores\ benchmark\ job)
execute(COMMAND /usr/bin/git -C "${Viskores_PERF_REPO}" config --local user.email do_not_email_the_robot@kitware.com)
execute(COMMAND /usr/bin/git -C "${Viskores_PERF_REPO}" add "${perf_report_name}")
execute(COMMAND /usr/bin/git -C "${Viskores_PERF_REPO}" commit -m "Added ${perf_report_name} record")
execute(COMMAND /usr/bin/git -C "${Viskores_PERF_REPO}" push origin records)
