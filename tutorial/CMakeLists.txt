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

set(CMAKE_PREFIX_PATH ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR})

cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(VTKm_tut)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(tut_io tut_io.cxx)
target_link_libraries(tut_io vtkm_filter vtkm_io)

add_executable(tut_contour tut_contour.cxx)
target_link_libraries(tut_contour vtkm_filter vtkm_io)

add_executable(tut_contour_2fields tut_contour_2fields.cxx)
target_link_libraries(tut_contour_2fields vtkm_filter vtkm_io)

add_executable(tut_2filters tut_2filters.cxx)
target_link_libraries(tut_2filters vtkm_filter vtkm_io)

add_executable(tut_mag_grad tut_mag_grad.cxx)
target_link_libraries(tut_mag_grad vtkm_filter vtkm_io)

if (VTKm_ENABLE_RENDERING)
  add_executable(tut_rendering tut_rendering.cxx)
  target_link_libraries(tut_rendering vtkm_filter vtkm_io vtkm_rendering)
endif()

add_executable(tut_error_handling tut_error_handling.cxx)
target_link_libraries(tut_error_handling vtkm_filter vtkm_io)

add_executable(tut_logging tut_logging.cxx)
target_link_libraries(tut_logging vtkm_filter vtkm_io)

add_executable(tut_point_to_cell tut_point_to_cell.cxx)
target_link_libraries(tut_point_to_cell vtkm_cont vtkm_filter vtkm_io)

add_executable(tut_extract_edges tut_extract_edges.cxx)
target_link_libraries(tut_extract_edges vtkm_cont vtkm_filter vtkm_io)


set(tutorial_targets tut_io tut_contour tut_contour_2fields tut_2filters tut_mag_grad tut_error_handling tut_logging tut_point_to_cell tut_extract_edges)

set(tutorial_sources tut_io.cxx tut_contour.cxx tut_contour_2fields.cxx tut_2filters.cxx tut_mag_grad.cxx tut_error_handling.cxx tut_logging.cxx tut_point_to_cell.cxx tut_extract_edges.cxx)

if (VTKm_ENABLE_RENDERING)
  list(APPEND tutorial_sources tut_rendering.cxx)
  list(APPEND tutorial_targets tut_rendering)
endif()


vtkm_add_target_information(${tutorial_targets}
                            DROP_UNUSED_SYMBOLS
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES
                             tutorial_sources)


# Copy the data file to be adjacent to the binaries
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:tut_io>/data/kitchen.vtk" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/data/kitchen.vtk")
