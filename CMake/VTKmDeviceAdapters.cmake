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

if(VTKm_ENABLE_TBB AND NOT TARGET vtkm::tbb)
  find_package(TBB REQUIRED)

  add_library(vtkm::tbb UNKNOWN IMPORTED)

  set_target_properties(vtkm::tbb PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${TBB_INCLUDE_DIRS}")

    if(TBB_LIBRARY_RELEASE)
      set_property(TARGET vtkm::tbb APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(vtkm::tbb PROPERTIES IMPORTED_LOCATION_RELEASE "${TBB_LIBRARY_RELEASE}")
    endif()

    if(TBB_LIBRARY_DEBUG)
      set_property(TARGET vtkm::tbb APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(vtkm::tbb PROPERTIES IMPORTED_LOCATION_DEBUG "${TBB_LIBRARY_DEBUG}")
    endif()

    if(NOT TBB_LIBRARY_RELEASE AND NOT TBB_LIBRARY_DEBUG)
      set_property(TARGET vtkm::tbb APPEND PROPERTY IMPORTED_LOCATION "${TBB_LIBRARY}")
    endif()
endif()


if(VTKm_ENABLE_CUDA AND NOT TARGET vtkm::cuda)
  cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
  enable_language(CUDA)

  add_library(vtkm::cuda UNKNOWN IMPORTED)

  # We can't have this location/lib empty, so we provide a location that is
  # valid and will have no effect on compilation
  list(GET CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES 0 VTKM_CUDA_LIBRARY)
  if(IS_ABSOLUTE "${VTKM_CUDA_LIBRARY}")
    set_property(TARGET vtkm::cuda APPEND PROPERTY IMPORTED_LOCATION "${VTKM_CUDA_LIBRARY}")
  else()
    find_library(cuda_lib
                 NAME ${VTKM_CUDA_LIBRARY}
                 PATHS ${CMAKE_CUDA_HOST_IMPLICIT_LINK_DIRECTORIES}
                 )
    set(VTKM_CUDA_LIBRARY ${cuda_lib})
    set_property(TARGET vtkm::cuda APPEND PROPERTY IMPORTED_LOCATION "${VTKM_CUDA_LIBRARY}")
  endif()

  set_target_properties(vtkm::cuda PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")

  #This is where we would add the -gencode flags so that all cuda code
  #way compiled properly

endif()
