##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#add the directory that contains the Viskores config file to the cmake
#path so that our tutorial can find Viskores
#Normally when running CMake, you need to set the Viskores_DIR to
#find the ViskoresConfig.cmake file. Because we already know where this
#file is, we can add the location to look to CMAKE_PREFIX_PATH.
set(CMAKE_PREFIX_PATH ${Viskores_BINARY_DIR}/${Viskores_INSTALL_CONFIG_DIR})

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(Viskores_tut)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)


if (Viskores_ENABLE_TUTORIALS)
 # Viskores tutorial targets expect viskores libraries to be namespaced with the prefix viskores::.
 # However, as the tutorial can also be built as part of the Viskores code,
 # those prefix are not added to the targets (This happens during the
 # installation). To workaround this issue here, we create IMPORTED libs linking
 # to the viskores libraries used by the tutorial targets with the expected viskores:: prefix.
 viskores_module_get_list(module_list)
 foreach(tgt IN LISTS module_list)
  if(TARGET ${tgt})
   # The reason of creating this phony IMPORTED libraries instead of making
   # ALIAS libraries is that ALIAS libraries are GLOBAL whereas IMPORTED are
   # local at the directory level where they are created. We do not want these
   # phony targets to be visible outside of the tutorial directory.
   viskores_target_mangle(tgt_name_mangled ${tgt})
   add_library("viskores::${tgt_name_mangled}" INTERFACE IMPORTED)
   target_link_libraries("viskores::${tgt_name_mangled}" INTERFACE ${tgt})
  endif()
 endforeach()
endif()

add_executable(io io.cxx)
target_link_libraries(io PRIVATE viskores::io)

add_executable(contour contour.cxx)
target_link_libraries(contour PRIVATE viskores::filter_core viskores::filter_contour viskores::io)

add_executable(contour_two_fields contour_two_fields.cxx)
target_link_libraries(contour_two_fields PRIVATE viskores::filter_core viskores::filter_contour viskores::io)

add_executable(two_filters two_filters.cxx)
target_link_libraries(two_filters PRIVATE viskores::filter_core viskores::filter_contour viskores::io)

add_executable(mag_grad mag_grad.cxx)
target_link_libraries(mag_grad PRIVATE viskores::filter_core viskores::filter_vector_analysis viskores::io viskores::worklet)
# Because mag_grad.cxx creates a worklet with code that
# runs on a GPU, it needs additional information.
viskores_add_target_information(mag_grad
  DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
  DEVICE_SOURCES mag_grad.cxx)

if (Viskores_ENABLE_RENDERING)
  add_executable(rendering rendering.cxx)
  target_link_libraries(rendering PRIVATE viskores::io viskores::rendering)
endif ()

add_executable(error_handling error_handling.cxx)
target_link_libraries(error_handling PRIVATE viskores::filter_core viskores::filter_contour viskores::io)

add_executable(logging logging.cxx)
target_link_libraries(logging PRIVATE viskores::io)

add_executable(point_to_cell point_to_cell.cxx)
target_link_libraries(point_to_cell PRIVATE viskores::worklet viskores::filter_core viskores::io)
viskores_add_target_information(point_to_cell
  DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
  DEVICE_SOURCES point_to_cell.cxx)

add_executable(extract_edges extract_edges.cxx)
target_link_libraries(extract_edges PRIVATE viskores::cont viskores::filter_core viskores::filter_contour viskores::worklet viskores::io)
viskores_add_target_information(extract_edges
  DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
  DEVICE_SOURCES extract_edges.cxx)

# Copy the data file to be adjacent to the binaries
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:mag_grad>/data/kitchen.vtk" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/data/kitchen.vtk")
