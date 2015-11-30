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

if (VTKm_Serial_initialize_complete)
  return()
endif (VTKm_Serial_initialize_complete)

vtkm_configure_device(Base)

if (VTKm_Base_FOUND)
  # Serial only relies on base configuration
  set(VTKm_Serial_FOUND TRUE)
else () # !VTKm_Base_FOUND
  set(VTKm_Serial_FOUND)
endif ()

#-----------------------------------------------------------------------------
# Set up the compiler flag optimizations
#-----------------------------------------------------------------------------
include(VTKmCompilerOptimizations)

if (VTKm_Serial_FOUND)
  set(VTKm_Serial_initialize_complete TRUE)
endif ()
