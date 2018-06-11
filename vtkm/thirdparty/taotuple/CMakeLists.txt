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
##  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2018 UT-Battelle, LLC.
##  Copyright 2018 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
add_library(vtkm_taotuple INTERFACE)

vtkm_get_kit_name(kit_name kit_dir)

# taotuple needs C++11
target_compile_features(vtkm_taotuple INTERFACE cxx_auto_type)

target_include_directories(vtkm_taotuple INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}/vtkm/thirdparty/taotuple>)

install(TARGETS vtkm_taotuple
  EXPORT ${VTKm_EXPORT_NAME})

## Install headers
if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY vtkmtaotuple
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
