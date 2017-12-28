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

set(headers
  ArrayManagerExecutionTBB.h
  DeviceAdapterAlgorithmTBB.h
  DeviceAdapterTagTBB.h
  FunctorsTBB.h
  VirtualObjectTransferTBB.h
  )

if (VTKm_ENABLE_TBB)
  if("${TBB_VERSION_MAJOR}.${TBB_VERSION_MINOR}" VERSION_LESS 4.3)
    if("${TBB_VERSION_MAJOR}.${TBB_VERSION_MINOR}" VERSION_LESS 4.0)
      # MESSAGE(FATAL_ERROR "VTK-m not tested with TBB before version 4.0. Please upgrade your TBB library.")
    endif()
  endif()
endif()

vtkm_declare_headers(parallel_sort.h TESTABLE OFF)
vtkm_declare_headers(${headers} TESTABLE ${VTKm_ENABLE_TBB})

#-----------------------------------------------------------------------------
if (NOT VTKm_ENABLE_TBB)
  return()
endif()

add_library(vtkm_cont_tbb OBJECT
  ArrayManagerExecutionTBB.cxx
  DeviceAdapterAlgorithmTBB.cxx
  )

target_compile_options(vtkm_cont_tbb PRIVATE $<TARGET_PROPERTY:vtkm_cont,INTERFACE_COMPILE_OPTIONS>)
target_compile_definitions(vtkm_cont_tbb PRIVATE $<TARGET_PROPERTY:vtkm_cont,INTERFACE_COMPILE_DEFINITIONS>)
target_compile_features(vtkm_cont_tbb PRIVATE $<TARGET_PROPERTY:vtkm_cont,INTERFACE_COMPILE_FEATURES>)
target_include_directories(vtkm_cont_tbb PRIVATE $<TARGET_PROPERTY:vtkm_cont,INTERFACE_INCLUDE_DIRECTORIES>)
set_property(TARGET vtkm_cont_tbb PROPERTY POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS})
