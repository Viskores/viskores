##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2015 Sandia Corporation.
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

# This is the base configuration for using the VTK-m library. All other
# device configurations rely on this.

if (VTKm_Base_initialize_complete)
  return()
endif (VTKm_Base_initialize_complete)

# Find the Boost library.
if(NOT Boost_FOUND)
  find_package(BoostHeaders ${VTKm_REQUIRED_BOOST_VERSION})
endif()

if (NOT Boost_FOUND)
  message(STATUS "Boost not found")
  set(VTKm_Base_FOUND)
else()
  set(VTKm_Base_FOUND TRUE)
endif ()

# Set up all these dependent packages (if they were all found).
if (VTKm_Base_FOUND)
  set(VTKm_INCLUDE_DIRS
    ${VTKm_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    )

  set(VTKm_Base_initialize_complete TRUE)
endif (VTKm_Base_FOUND)
