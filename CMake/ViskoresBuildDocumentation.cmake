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
# Find Doxygen
#-----------------------------------------------------------------------------
find_package(Doxygen REQUIRED)

#-----------------------------------------------------------------------------
# Function to turn CMake booleans to `YES` or `NO` as expected by Doxygen
#-----------------------------------------------------------------------------
function(to_yes_no variable)
  if(${variable})
    set(${variable} YES PARENT_SCOPE)
  else()
    set(${variable} NO PARENT_SCOPE)
  endif()
endfunction()

#-----------------------------------------------------------------------------
# Configure Doxygen
#-----------------------------------------------------------------------------
set(Viskores_DOXYGEN_HAVE_DOT ${DOXYGEN_DOT_FOUND})
set(Viskores_DOXYGEN_DOT_PATH ${DOXYGEN_DOT_PATH})
set(Viskores_DOXYFILE ${CMAKE_CURRENT_BINARY_DIR}/docs/doxyfile)

to_yes_no(Viskores_ENABLE_USERS_GUIDE)
to_yes_no(Viskores_Doxygen_HTML_output)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMake/doxyfile.in ${Viskores_DOXYFILE}
    @ONLY)

#-----------------------------------------------------------------------------
# Run Doxygen
#-----------------------------------------------------------------------------
if(WIN32)
  set(doxygen_redirect NUL)
else()
  set(doxygen_redirect /dev/null)
endif()
add_custom_command(
  OUTPUT ${Viskores_BINARY_DIR}/docs/doxygen
  COMMAND ${DOXYGEN_EXECUTABLE} ${Viskores_DOXYFILE} > ${doxygen_redirect}
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/CMake/doxyfile.in
  DEPENDS ${Viskores_DOXYFILE}
  COMMENT "Generating Viskores Documentation"
)
add_custom_target(ViskoresDoxygenDocs
  ALL
  DEPENDS ${Viskores_BINARY_DIR}/docs/doxygen
)
