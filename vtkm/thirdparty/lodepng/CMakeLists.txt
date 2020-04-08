##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(headers
  vtkmlodepng/lodepng.h
)
set(sources
  vtkmlodepng/lodepng.cpp
)
# vtkm_library(NAME vtkm_lodepng HEADERS ${headers} SOURCES ${sources})
add_library(vtkm_lodepng STATIC ${headers} ${sources})
if(DEFINED VTKm_CUSTOM_LIBRARY_SUFFIX)
  set(_lib_suffix "${VTKm_CUSTOM_LIBRARY_SUFFIX}")
else()
  set(_lib_suffix "-${VTKm_VERSION_MAJOR}.${VTKm_VERSION_MINOR}")
endif()
set_target_properties(vtkm_lodepng
  PROPERTIES
  OUTPUT_NAME "vtkm_lodepng${_lib_suffix}"
  LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
  ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
  RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
  VERSION 1
  SOVERSION 1
)
vtkm_generate_export_header(vtkm_lodepng)
target_include_directories(vtkm_lodepng INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}/vtkm/thirdparty/lodepng>
)
install(DIRECTORY vtkmlodepng
  DESTINATION "${VTKm_INSTALL_INCLUDE_DIR}/vtkm/thirdparty/lodepng"
)

install(TARGETS vtkm_lodepng
  EXPORT ${VTKm_EXPORT_NAME}
  LIBRARY DESTINATION ${VTKm_INSTALL_LIB_DIR}
  ARCHIVE DESTINATION ${VTKm_INSTALL_LIB_DIR}
  RUNTIME DESTINATION ${VTKm_INSTALL_BIN_DIR}
  COMPONENT libraries
)
