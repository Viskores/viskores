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
##  Copyright 2014. Los Alamos National Security
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

if (VTKm_Serial_initialize_complete)
  return()
endif (VTKm_Serial_initialize_complete)

# Find the Boost library.
if (NOT VTKm_Serial_FOUND)
  if(NOT Boost_FOUND)
    find_package(BoostHeaders ${VTKm_REQUIRED_BOOST_VERSION})
  endif()

  if (NOT Boost_FOUND)
    message(STATUS "Boost not found")
    set(VTKm_Serial_FOUND FALSE)
  else(NOT Boost_FOUND)
    set(VTKm_Serial_FOUND TRUE)
  endif (NOT Boost_FOUND)
endif (NOT VTKm_Serial_FOUND)

# Set up all these dependent packages (if they were all found).
if (VTKm_Serial_FOUND)
  include_directories(
    ${Boost_INCLUDE_DIRS}
    ${VTKm_INCLUDE_DIRS}
    )

  set(VTKm_Serial_initialize_complete TRUE)
endif (VTKm_Serial_FOUND)
