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

  # Need to sort this out still...cuda_add_library doesn't support OBJECT
  # libraries:
  #
#  cuda_add_library(vtkm_cont_cuda OBJECT ArrayManagerExecutionCuda.cu)
#  set_property(TARGET vtkm_cont_cuda PROPERTY POSITION_INDEPENDENT_CODE ON)
#  target_compile_features(vtkm_cont_cuda PRIVATE cxx_auto_type)
#  target_include_directories(vtkm_cont_cuda PRIVATE
#      "${VTKm_SOURCE_DIR}"
#      "${VTKm_BINARY_DIR}/include"
#      ${VTKm_BACKEND_INCLUDE_DIRS}
#    )
endif()
