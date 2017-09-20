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
find_package(VTKm QUIET REQUIRED
  OPTIONAL_COMPONENTS Serial CUDA TBB OSMesa Rendering
  )


if(VTKm_OSMesa_FOUND AND VTKm_Rendering_FOUND)
  if(VTKm_CUDA_FOUND)
    # Cuda compiles do not respect target_include_directories
    cuda_include_directories(${VTKm_INCLUDE_DIRS})
    cuda_add_executable(Demo Demo.cu)
  else()
    add_executable(Demo Demo.cxx)
  endif()


  #need to setup the default device adapter.
  target_include_directories(Demo PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(Demo PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(Demo PRIVATE ${VTKm_COMPILE_OPTIONS})

  #when we are using TBB pass in the default device adapter as
  #a compile definition to make sure we build with TBB selected
  if(VTKm_TBB_FOUND)
    target_compile_definitions(Demo
        PRIVATE "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
  endif()

endif()
