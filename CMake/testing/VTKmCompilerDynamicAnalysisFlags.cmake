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

function(vtkm_check_sanitizer_build)

  # each line is a separate entry
  set(blacklist_file_content "
src:${VTKm_SOURCE_DIR}/vtkm/thirdparty/
")
  set (sanitizer_blacklist "${VTKm_BINARY_DIR}/sanitizer_blacklist.txt")
  file(WRITE "${sanitizer_blacklist}" "${blacklist_file_content}")

  set(sanitizer_flags )
  foreach(sanitizer IN LISTS VTKm_USE_SANITIZER)
    string(APPEND sanitizer_flags "-fsanitize=${sanitizer} ")
  endforeach()
  # Add the compiler flags for blacklist
  if(VTKM_COMPILER_IS_CLANG)
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

if(VTKM_COMPILER_IS_CLANG OR VTKM_COMPILER_IS_GNU)
  vtkm_option(VTKm_ENABLE_SANITIZER "Build with sanitizer support." OFF)
  mark_as_advanced(VTKm_ENABLE_SANITIZER)

  set(VTKm_USE_SANITIZER "address" CACHE STRING "The sanitizer to use")
  mark_as_advanced(VTKm_USE_SANITIZER)

  if(VTKm_ENABLE_SANITIZER)
    vtkm_check_sanitizer_build()
  endif()

endif()
