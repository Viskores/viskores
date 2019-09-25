##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
add_library(vtkm_vtkc INTERFACE)

vtkm_get_kit_name(kit_name kit_dir)

# vtkc needs C++11
target_compile_features(vtkm_vtkc INTERFACE cxx_std_11)

target_include_directories(vtkm_vtkc INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vtkmvtkc>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}/vtkm/thirdparty/vtkc/vtkmvtkc>)

install(TARGETS vtkm_vtkc
  EXPORT ${VTKm_EXPORT_NAME})

## Install headers
if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY vtkmvtkc
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
