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
vtkm_configure_component_OSMESA()
if(VTKm_OSMESA_FOUND)

  if(VTKm_CUDA_FOUND)
    cuda_add_executable(Demo Demo.cu)
  else()
    add_executable(Demo Demo.cxx)
  endif()

  target_include_directories(Demo PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(Demo ${VTKm_LIBRARIES})
  target_compile_options(Demo PRIVATE ${VTKm_COMPILE_OPTIONS})

endif()
