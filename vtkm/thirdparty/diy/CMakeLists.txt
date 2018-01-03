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
##  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2017 UT-Battelle, LLC.
##  Copyright 2017 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
add_library(diy INTERFACE)

vtkm_get_kit_name(kit_name kit_dir)

# diy needs C++11
target_compile_features(diy INTERFACE cxx_auto_type)

# placeholder to support external DIY
set(VTKM_USE_EXTERNAL_DIY OFF)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h)

target_include_directories(diy INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}>)

# presently, this dependency is required. Make it optional in the future.
set(arg)
foreach(apath IN LISTS MPI_C_INCLUDE_PATH MPI_CXX_INCLUDE_PATH)
  list(APPEND arg $<BUILD_INTERFACE:${apath}>)
endforeach()
target_include_directories(diy INTERFACE ${arg})

target_link_libraries(diy INTERFACE
  $<BUILD_INTERFACE:${MPI_C_LIBRARIES}>
  $<BUILD_INTERFACE:${MPI_CXX_LIBRARIES}>)

if(MPI_C_COMPILE_DEFINITIONS)
  target_compile_definitions(diy INTERFACE
    $<$<COMPILE_LANGUAGE:C>:${MPI_C_COMPILE_DEFINITIONS}>)
endif()
if(MPI_CXX_COMPILE_DEFNITIONS)
  target_compile_definitions(diy INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:${MPI_CXX_COMPILE_DEFNITIONS>)
endif()

install(TARGETS diy
  EXPORT ${VTKm_EXPORT_NAME})

## Install headers
install(DIRECTORY vtkmdiy
  DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
install(FILES ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
  DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
