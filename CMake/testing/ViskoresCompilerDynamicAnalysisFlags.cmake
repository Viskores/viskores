##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#-----------------------------------------------------------------------------
# check if this is a sanitizer build. If so, set up the environment.

function(viskores_check_sanitizer_build)

  # each line is a separate entry
  set(blacklist_file_content "
src:${Viskores_SOURCE_DIR}/viskores/thirdparty/
")
  set (sanitizer_blacklist "${Viskores_BINARY_DIR}/sanitizer_blacklist.txt")
  file(WRITE "${sanitizer_blacklist}" "${blacklist_file_content}")

  set(sanitizer_flags )
  foreach(sanitizer IN LISTS Viskores_USE_SANITIZER)
    string(APPEND sanitizer_flags "-fsanitize=${sanitizer} ")
  endforeach()
  # Add the compiler flags for blacklist
  if(VISKORES_COMPILER_IS_CLANG)
    string(APPEND sanitizer_flags "\"-fsanitize-blacklist=${sanitizer_blacklist}\"")
  endif()
  foreach (entity C CXX SHARED_LINKER EXE_LINKER)
    set (CMAKE_${entity}_FLAGS "${CMAKE_${entity}_FLAGS} ${sanitizer_flags}" PARENT_SCOPE)
  endforeach ()

endfunction()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
   CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

if(VISKORES_COMPILER_IS_CLANG OR VISKORES_COMPILER_IS_GNU)
  viskores_option(Viskores_ENABLE_SANITIZER "Build with sanitizer support." OFF)
  mark_as_advanced(Viskores_ENABLE_SANITIZER)

  set(Viskores_USE_SANITIZER "address" CACHE STRING "The sanitizer to use")
  mark_as_advanced(Viskores_USE_SANITIZER)

  if(Viskores_ENABLE_SANITIZER)
    viskores_check_sanitizer_build()
  endif()

endif()
