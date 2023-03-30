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
#path so that our tutorial can find VTK-m
#Normally when running CMake, you need to set the VTKm_DIR to
#find the VTKmConfig.cmake file. Because we already know where this
#file is, we can add the location to look to CMAKE_PREFIX_PATH.
set(CMAKE_PREFIX_PATH ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR})

cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(VTKm_tut)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)


if (VTKm_ENABLE_TUTORIALS)
 # VTK-m tutorial targets expect vtkm libraries to be namespaced with the prefix vtkm::.
 # However, as the tutorial can also be built as part of the VTK-m code,
 # those prefix are not added to the targets (This happens during the
 # installation). To workaround this issue here, we create IMPORTED libs linking
 # to the vtkm libraries used by the tutorial targets with the expected vtkm:: prefix.
 vtkm_module_get_list(module_list)
 foreach(tgt IN LISTS module_list)
  if(TARGET ${tgt})
   # The reason of creating this phony IMPORTED libraries instead of making
   # ALIAS libraries is that ALIAS libraries are GLOBAL whereas IMPORTED are
   # local at the directory level where they are created. We do not want these
   # phony targets to be visible outside of the tutorial directory.
   vtkm_target_mangle(tgt_name_mangled ${tgt})
   add_library("vtkm::${tgt_name_mangled}" INTERFACE IMPORTED)
   target_link_libraries("vtkm::${tgt_name_mangled}" INTERFACE ${tgt})
  endif()
 endforeach()
endif()

add_executable(io io.cxx)
target_link_libraries(io vtkm::io)

add_executable(contour contour.cxx)
target_link_libraries(contour vtkm::filter_core vtkm::filter_contour vtkm::io)

add_executable(contour_two_fields contour_two_fields.cxx)
target_link_libraries(contour_two_fields vtkm::filter_core vtkm::filter_contour vtkm::io)

add_executable(two_filters two_filters.cxx)
target_link_libraries(two_filters vtkm::filter_core vtkm::filter_contour vtkm::io)

add_executable(mag_grad mag_grad.cxx)
target_link_libraries(mag_grad vtkm::filter_core vtkm::filter_vector_analysis vtkm::io)
# Because mag_grad.cxx creates a worklet with code that
# runs on a GPU, it needs additional information.
vtkm_add_target_information(mag_grad
  DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
  DEVICE_SOURCES mag_grad.cxx)

if (VTKm_ENABLE_RENDERING)
  add_executable(rendering rendering.cxx)
  target_link_libraries(rendering vtkm::io vtkm::rendering)
endif ()

add_executable(error_handling error_handling.cxx)
target_link_libraries(error_handling vtkm::filter_core vtkm::filter_contour vtkm::io)

add_executable(logging logging.cxx)
target_link_libraries(logging vtkm::io)

add_executable(point_to_cell point_to_cell.cxx)
target_link_libraries(point_to_cell vtkm::worklet vtkm::filter_core vtkm::io)
vtkm_add_target_information(point_to_cell
  DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
  DEVICE_SOURCES point_to_cell.cxx)

add_executable(extract_edges extract_edges.cxx)
target_link_libraries(extract_edges vtkm::cont vtkm::filter_core vtkm::filter_contour vtkm::worklet vtkm::io)
vtkm_add_target_information(extract_edges
  DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
  DEVICE_SOURCES extract_edges.cxx)

# Copy the data file to be adjacent to the binaries
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:mag_grad>/data/kitchen.vtk" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/data/kitchen.vtk")
