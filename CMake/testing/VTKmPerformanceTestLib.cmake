##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

macro(REQUIRE_FLAG flag)
  if (NOT DEFINED ${flag})
    message(FATAL_ERROR "Need to pass the ${flag}")
  endif()
endmacro(REQUIRE_FLAG)

macro(REQUIRE_FLAG_MUTABLE flag)
  REQUIRE_FLAG(${flag})

  # Env var overrides default value
  if (DEFINED ENV{${flag}})
    set(${flag} "$ENV{${flag}}")
  endif()
endmacro(REQUIRE_FLAG_MUTABLE)

macro(execute)
  execute_process(
    ${ARGV}
    COMMAND_ECHO STDOUT
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE
    COMMAND_ERROR_IS_FATAL ANY
    )
endmacro()

message("CTEST_FULL_OUTPUT")
