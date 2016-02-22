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
##  Copyright 2015 Sandia Corporation.
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================

if(VTKm_CUDA_FOUND)
  cuda_add_executable(MultiBackend MultiBackend.cu)
else()
  add_executable(MultiBackend MultiBackend.cxx)
endif()

target_compile_options(MultiBackend PRIVATE ${VTKm_COMPILE_OPTIONS})
if(VTKm_TBB_FOUND)
   target_link_libraries(MultiBackend  ${VTKm_LIBRARIES})
endif()
