##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#add the directory that contains the VTK-m config file to the cmake
#path so that our examples can find VTK-m
#Normally when running CMake, you need to set the VTKm_DIR to
#find the VTKmConfig.cmake file. Because we already know where this
#file is, we can add the location to look to CMAKE_PREFIX_PATH.

set(CMAKE_PREFIX_PATH ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR})

cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(VTKm_tut)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(io io.cxx)
target_link_libraries(io vtkm_filter vtkm_io)

add_executable(contour contour.cxx)
target_link_libraries(contour vtkm_filter vtkm_io)

add_executable(contour_two_fields contour_two_fields.cxx)
target_link_libraries(contour_two_fields vtkm_filter vtkm_io)

add_executable(two_filters two_filters.cxx)
target_link_libraries(two_filters vtkm_filter vtkm_io)

add_executable(mag_grad mag_grad.cxx)
target_link_libraries(mag_grad vtkm_filter vtkm_io)
# Because mag_grad.cxx creates a worklet with code that
# runs on a GPU, it needs additional information.
vtkm_add_target_information(mag_grad
  DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
  DEVICE_SOURCES mag_grad.cxx)

if (VTKm_ENABLE_RENDERING)
  add_executable(rendering rendering.cxx)
  target_link_libraries(rendering vtkm_filter vtkm_io vtkm_rendering)
endif ()

add_executable(error_handling error_handling.cxx)
target_link_libraries(error_handling vtkm_filter vtkm_io)

add_executable(logging logging.cxx)
target_link_libraries(logging vtkm_filter vtkm_io)

add_executable(point_to_cell point_to_cell.cxx)
target_link_libraries(point_to_cell vtkm_cont vtkm_filter vtkm_io)
vtkm_add_target_information(point_to_cell
  DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
  DEVICE_SOURCES point_to_cell.cxx)

add_executable(extract_edges extract_edges.cxx)
target_link_libraries(extract_edges vtkm_cont vtkm_filter vtkm_io)
vtkm_add_target_information(extract_edges
  DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
  DEVICE_SOURCES extract_edges.cxx)

# Copy the data file to be adjacent to the binaries
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:mag_grad>/data/kitchen.vtk" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/data/kitchen.vtk")
