##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
