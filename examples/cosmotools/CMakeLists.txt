##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
#Find the VTK-m package
find_package(VTKm REQUIRED QUIET
  OPTIONAL_COMPONENTS Serial CUDA TBB
  )

add_executable(CosmoCenterFinder_SERIAL CosmoCenterFinder.cxx)
target_include_directories(CosmoCenterFinder_SERIAL PRIVATE ${VTKm_INCLUDE_DIRS})
target_link_libraries(CosmoCenterFinder_SERIAL PRIVATE ${VTKm_LIBRARIES})
target_compile_options(CosmoCenterFinder_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

add_executable(CosmoHaloFinder_SERIAL CosmoHaloFinder.cxx)
target_include_directories(CosmoHaloFinder_SERIAL PRIVATE ${VTKm_INCLUDE_DIRS})
target_link_libraries(CosmoHaloFinder_SERIAL PRIVATE ${VTKm_LIBRARIES})
target_compile_options(CosmoHaloFinder_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

if(VTKm_CUDA_FOUND)
  # Cuda compiles do not respect target_include_directories
  cuda_include_directories(${VTKm_INCLUDE_DIRS})

  cuda_add_executable(CosmoCenterFinder_CUDA CosmoCenterFinder.cu)
  target_include_directories(CosmoCenterFinder_CUDA PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(CosmoCenterFinder_CUDA PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(CosmoCenterFinder_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})

  cuda_add_executable(CosmoHaloFinder_CUDA CosmoHaloFinder.cu)
  target_include_directories(CosmoHaloFinder_CUDA PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(CosmoHaloFinder_CUDA PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(CosmoHaloFinder_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})
endif()

if(VTKm_TBB_FOUND)
  add_executable(CosmoCenterFinder_TBB CosmoCenterFinderTBB.cxx)
  target_include_directories(CosmoCenterFinder_TBB PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(CosmoCenterFinder_TBB PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(CosmoCenterFinder_TBB PRIVATE PRIVATE ${VTKm_COMPILE_OPTIONS})

  add_executable(CosmoHaloFinder_TBB CosmoHaloFinderTBB.cxx)
  target_include_directories(CosmoHaloFinder_TBB PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(CosmoHaloFinder_TBB PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(CosmoHaloFinder_TBB PRIVATE PRIVATE ${VTKm_COMPILE_OPTIONS})
endif()
