##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
function(vtkm_build_documentation)
  add_custom_command(
    OUTPUT ${VTKm_BINARY_DIR}/docs/doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${VTKm_DOXYFILE} > /dev/null
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/CMake/doxyfile.in
    DEPENDS ${VTKm_DOXYFILE}
    COMMENT "Generating VTKm Documentation"
  )
  add_custom_target(VTKmDoxygenDocs
    ALL
    DEPENDS ${VTKm_BINARY_DIR}/docs/doxygen
  )
endfunction(vtkm_build_documentation)
