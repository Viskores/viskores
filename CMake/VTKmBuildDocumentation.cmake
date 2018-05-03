##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
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
