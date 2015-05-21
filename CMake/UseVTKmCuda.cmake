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

if (VTKm_Cuda_initialize_complete)
  return()
endif (VTKm_Cuda_initialize_complete)

set(VTKm_Cuda_FOUND ${VTKm_ENABLE_CUDA})
if (NOT VTKm_Cuda_FOUND)
  message(STATUS "This build of VTKm does not include Cuda.")
endif (NOT VTKm_Cuda_FOUND)

# Find the Boost library.
if (VTKm_Cuda_FOUND)
  if(NOT Boost_FOUND)
    find_package(BoostHeaders ${VTKm_REQUIRED_BOOST_VERSION})
  endif()

  if (NOT Boost_FOUND)
    message(STATUS "Boost not found")
    set(VTKm_Cuda_FOUND)
  endif (NOT Boost_FOUND)
endif (VTKm_Cuda_FOUND)

#-----------------------------------------------------------------------------
# Find CUDA library.
#-----------------------------------------------------------------------------
if (VTKm_Cuda_FOUND)
  find_package(CUDA)
  mark_as_advanced(CUDA_BUILD_CUBIN
                   CUDA_BUILD_EMULATION
                   CUDA_HOST_COMPILER
                   CUDA_SDK_ROOT_DIR
                   CUDA_SEPARABLE_COMPILATION
                   CUDA_TOOLKIT_ROOT_DIR
                   CUDA_VERBOSE_BUILD
                   )

  if (NOT CUDA_FOUND)
    message(STATUS "CUDA not found")
    set(VTKm_Cuda_FOUND)
  endif (NOT CUDA_FOUND)
endif ()

#-----------------------------------------------------------------------------
# Find Thrust library.
#-----------------------------------------------------------------------------
if (VTKm_Cuda_FOUND)
  find_package(Thrust)

  if (NOT THRUST_FOUND)
    message(STATUS "Thrust not found")
    set(VTKm_Cuda_FOUND)
  endif (NOT THRUST_FOUND)
endif ()

#-----------------------------------------------------------------------------
# Set up all these dependent packages (if they were all found).
#-----------------------------------------------------------------------------
if (VTKm_Cuda_FOUND)
  cuda_include_directories(
    ${Boost_INCLUDE_DIRS}
    ${THRUST_INCLUDE_DIRS}
    ${VTKm_INCLUDE_DIRS}
    )
  set(VTKm_Cuda_initialize_complete TRUE)
endif (VTKm_Cuda_FOUND)
