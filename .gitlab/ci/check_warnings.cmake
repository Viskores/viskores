##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

# Find the path the logs from the last configure
set(cnf_log_path "${CMAKE_SOURCE_DIR}/build/Testing/Temporary/LastConfigure*.log")
file(GLOB cnf_log_files ${cnf_log_path})

foreach(file IN LISTS cnf_log_files)
  file(STRINGS ${file} lines)
  string(FIND "${lines}" "Warning" line)
  if (NOT ${line} EQUAL "-1")
    message(FATAL_ERROR "Configure warnings detected, please check cdash-commit job")
  endif()
endforeach()

# `compile_num_warnings` contains a single integer symbolizing the number of
# warnings of the last build.
set(bld_log_path "${CMAKE_SOURCE_DIR}/build/compile_num_warnings.log")
file(STRINGS "${bld_log_path}" output)
if (NOT "${output}" STREQUAL "0")
  message(FATAL_ERROR "Build warnings detected, please check cdash-commit job")
endif()
