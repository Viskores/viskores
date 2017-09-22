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
  OPTIONAL_COMPONENTS Serial CUDA TBB OpenGL GLUT
  )

if(VTKm_CUDA_FOUND)
  vtkm_disable_troublesome_thrust_warnings()
  # Cuda compiles do not respect target_include_directories
  cuda_include_directories(${VTKm_INCLUDE_DIRS})
  cuda_add_executable(UnifiedMemory_CUDA UnifiedMemory.cu)
  target_link_libraries(UnifiedMemory_CUDA PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(UnifiedMemory_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})
endif()

