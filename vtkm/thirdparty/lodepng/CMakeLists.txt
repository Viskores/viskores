##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
add_library(vtkm_lodepng INTERFACE)

vtkm_get_kit_name(kit_name kit_dir)

target_include_directories(vtkm_lodepng INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}/vtkm/thirdparty/lodepng>)

install(TARGETS vtkm_lodepng
  EXPORT ${VTKm_EXPORT_NAME})

## Install headers
if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY vtkmlodepng
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
