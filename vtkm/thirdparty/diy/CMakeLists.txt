##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
add_library(vtkm_diy INTERFACE)

vtkm_get_kit_name(kit_name kit_dir)

# diy needs C++11
target_compile_features(vtkm_diy INTERFACE cxx_std_11)

# placeholder to support external DIY
set(VTKM_USE_EXTERNAL_DIY OFF)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h)

target_include_directories(vtkm_diy INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}/vtkm/thirdparty/diy>)

if(VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_diy INTERFACE MPI::MPI_CXX)
endif()

install(TARGETS vtkm_diy
  EXPORT ${VTKm_EXPORT_NAME})

  ## Install headers
if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY vtkmdiy
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
  install(FILES
    ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
    ${CMAKE_CURRENT_SOURCE_DIR}/diy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization.h
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
