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
# Configure Doxygen
#-----------------------------------------------------------------------------
set(VTKm_DOXYGEN_HAVE_DOT ${DOXYGEN_DOT_FOUND})
set(VTKm_DOXYGEN_DOT_PATH ${DOXYGEN_DOT_PATH})
set(VTKm_DOXYFILE ${CMAKE_CURRENT_BINARY_DIR}/docs/doxyfile)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMake/doxyfile.in ${VTKm_DOXYFILE}
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
  OUTPUT ${VTKm_BINARY_DIR}/docs/doxygen
  COMMAND ${DOXYGEN_EXECUTABLE} ${VTKm_DOXYFILE} > ${doxygen_redirect}
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/CMake/doxyfile.in
  DEPENDS ${VTKm_DOXYFILE}
  COMMENT "Generating VTKm Documentation"
)
add_custom_target(VTKmDoxygenDocs
  ALL
  DEPENDS ${VTKm_BINARY_DIR}/docs/doxygen
)
