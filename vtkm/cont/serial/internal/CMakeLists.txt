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
  ArrayManagerExecutionSerial.h
  DeviceAdapterAlgorithmSerial.h
  DeviceAdapterTagSerial.h
  VirtualObjectTransferSerial.h
  )
vtkm_declare_headers(${headers})

add_library(vtkm_cont_serial OBJECT
  ArrayManagerExecutionSerial.cxx
  DeviceAdapterAlgorithmSerial.cxx
  )


vtkm_setup_msvc_properties(vtkm_cont_serial)

target_compile_features(vtkm_cont_serial PRIVATE cxx_auto_type)
target_compile_definitions(vtkm_cont_serial PRIVATE vtkm_cont_EXPORTS)
if(BUILD_SHARED_LIBS)
  set_property(TARGET vtkm_cont_serial PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

target_include_directories(vtkm_cont_serial PRIVATE
    ${VTKm_BACKEND_INCLUDE_DIRS}
    ${VTKm_SOURCE_DIR}
    ${VTKm_BINARY_INCLUDE_DIR}
  )
