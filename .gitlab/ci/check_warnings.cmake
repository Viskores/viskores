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

set(configure_warning_exceptions
  ".*CMake Warning at CMake/VTKmDetermineVersion.cmake.*"
  )

# Find the path of the logs from the last configure
set(cnf_log_path "${CMAKE_SOURCE_DIR}/build/Testing/Temporary/LastConfigure*.log")
file(GLOB cnf_log_files ${cnf_log_path})

# Check for warnings during the configure phase
foreach(file IN LISTS cnf_log_files)
  file(STRINGS ${file} lines)
  foreach(line IN LISTS lines)
    if ("${line}" MATCHES "Warning|WARNING|warning")
      set(exception_matches FALSE)
      foreach(exception IN LISTS configure_warning_exceptions)
        if (${line} MATCHES "${exception}")
          set(exception_matches TRUE)
          break()
        endif()
      endforeach()
      if (NOT exception_matches)
        message(FATAL_ERROR "Configure warnings detected, please check cdash-commit job: ${line}")
      endif()
    endif()
  endforeach()
endforeach()

# `compile_num_warnings` contains a single integer symbolizing the number of
# warnings of the last build.
set(bld_log_path "${CMAKE_SOURCE_DIR}/build/compile_num_warnings.log")
file(STRINGS "${bld_log_path}" output)
if (NOT "${output}" STREQUAL "0")
  message(FATAL_ERROR "Build warnings detected, please check cdash-commit job")
endif()
