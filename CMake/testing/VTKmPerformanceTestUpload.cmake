##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

include("${VTKm_SOURCE_DIR}/CMake/testing/VTKmPerformanceTestLib.cmake")

REQUIRE_FLAG("VTKm_PERF_COMPARE_JSON")
REQUIRE_FLAG_MUTABLE("VTKm_PERF_REPO")

file(COPY "${VTKm_PERF_COMPARE_JSON}" DESTINATION "${VTKm_PERF_REPO}/")
get_filename_component(perf_report_name "${VTKm_PERF_COMPARE_JSON}" NAME)

execute(COMMAND /usr/bin/git -C "${VTKm_PERF_REPO}" config --local user.name vtk-m\ benchmark\ job)
execute(COMMAND /usr/bin/git -C "${VTKm_PERF_REPO}" config --local user.email do_not_email_the_robot@kitware.com)
execute(COMMAND /usr/bin/git -C "${VTKm_PERF_REPO}" add "${perf_report_name}")
execute(COMMAND /usr/bin/git -C "${VTKm_PERF_REPO}" commit -m "Added ${perf_report_name} record")
execute(COMMAND /usr/bin/git -C "${VTKm_PERF_REPO}" push origin records)
