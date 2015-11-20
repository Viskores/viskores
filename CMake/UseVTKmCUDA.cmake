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

if (VTKm_CUDA_initialize_complete)
  return()
endif (VTKm_CUDA_initialize_complete)

vtkm_configure_device(Base)

if (VTKm_Base_FOUND)

  set(VTKm_CUDA_FOUND ${VTKm_ENABLE_CUDA})
  if (NOT VTKm_CUDA_FOUND)
    message(STATUS "This build of VTK-m does not include CUDA.")
  endif ()

  #---------------------------------------------------------------------------
  # Find CUDA library.
  #---------------------------------------------------------------------------
  if (VTKm_CUDA_FOUND)
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
      set(VTKm_CUDA_FOUND)
    endif ()
  endif ()

  #---------------------------------------------------------------------------
  # Find Thrust library.
  #---------------------------------------------------------------------------
  if (VTKm_CUDA_FOUND)
    find_package(Thrust)

    if (NOT THRUST_FOUND)
      message(STATUS "Thrust not found")
      set(VTKm_CUDA_FOUND)
    endif ()
  endif ()

endif () # VTKm_Base_FOUND

#-----------------------------------------------------------------------------
# Set up all these dependent packages (if they were all found).
#-----------------------------------------------------------------------------
if (VTKm_CUDA_FOUND)
  set(VTKm_INCLUDE_DIRS
    ${VTKm_INCLUDE_DIRS}
    ${THRUST_INCLUDE_DIRS}
    )

  set(VTKm_CUDA_initialize_complete TRUE)
endif (VTKm_CUDA_FOUND)
