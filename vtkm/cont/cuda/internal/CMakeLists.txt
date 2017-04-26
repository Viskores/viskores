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

set(headers
  ArrayManagerExecutionCuda.h
  ArrayManagerExecutionThrustDevice.h
  DeviceAdapterAlgorithmCuda.h
  DeviceAdapterAlgorithmThrust.h
  DeviceAdapterTagCuda.h
  MakeThrustIterator.h
  ThrustExceptionHandler.h
  )

vtkm_declare_headers(CUDA ${headers} TESTABLE ${VTKm_ENABLE_CUDA})

#-----------------------------------------------------------------------------
if (VTKm_ENABLE_CUDA)
  add_subdirectory(testing)

  #todo we need to add a custom target and feed that as a dependency
  #for vtkm_cont
  cuda_include_directories(${VTKm_SOURCE_DIR}
                           ${VTKm_BINARY_DIR}/include
                           ${VTKm_BACKEND_INCLUDE_DIRS}
                           )

  set(compile_options -Dvtkm_cont_EXPORTS)
  if(BUILD_SHARED_LIBS AND NOT WIN32)
    list(APPEND compile_options -Xcompiler=${CMAKE_CXX_COMPILE_OPTIONS_VISIBILITY}hidden)
    list(APPEND compile_options -Xcompiler=-fPIC)
  endif()

  cuda_compile(vtkm_cont_cuda_object_files ArrayManagerExecutionCuda.cu
               OPTIONS "${compile_options}")

  #Setup the dependency chain for the custom object build so that
  add_custom_target(vtkm_cont_cuda DEPENDS ${vtkm_cont_cuda_object_files})

  set_property( GLOBAL
                PROPERTY vtkm_cont_cuda_object_files ${vtkm_cont_cuda_object_files})

endif()
