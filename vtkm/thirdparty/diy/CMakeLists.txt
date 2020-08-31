##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
vtkm_get_kit_name(kit_name kit_dir)

include(CMakeDependentOption)
if (NOT DEFINED VTKm_ENABLE_DIY_NOMPI)
  cmake_dependent_option(
    VTKm_ENABLE_DIY_NOMPI "Also build DIY without mpi" OFF "VTKm_ENABLE_MPI" OFF)
endif()

if (VTKm_ENABLE_DIY_NOMPI AND
    (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC") AND
    BUILD_SHARED_LIBS)
  message(WARNING "VTKm_ENABLE_DIY_NOMPI support with MSVC shared builds is experimental and may not work.")
endif()

## Configure diy
set(build_examples OFF)
set(build_tests OFF)

set(threads OFF)
set(log OFF)
set(profile OFF)
set(caliper OFF)

set(build_diy_mpi_lib ON)

set(mpi OFF)
set(build_diy_nompi_lib OFF)
if (VTKm_ENABLE_MPI)
  set(mpi ON)
endif()
if (VTKm_ENABLE_DIY_NOMPI)
  set(build_diy_nompi_lib ON)
endif()

mark_as_advanced(FORCE caliper log profile wrapped_mpi)

set(diy_prefix "vtkmdiy")
set(diy_install_include_dir ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/vtkmdiy/include)
set(diy_install_bin_dir ${VTKm_INSTALL_BIN_DIR})
set(diy_install_lib_dir ${VTKm_INSTALL_LIB_DIR})
set(diy_export_name ${VTKm_EXPORT_NAME})
if (VTKm_INSTALL_ONLY_LIBRARIES)
  set(diy_install_only_libraries)
endif()
set(diy_dont_install_export)

# placeholder to support external DIY
set(VTKM_USE_EXTERNAL_DIY OFF)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h)

function(vtkm_diy_set_target_output_directory target)
  set_property(TARGET ${target} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${target} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${target} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH})
endfunction()

#-----------------------------------------------------------------------------
add_subdirectory(vtkmdiy)

# move diy libraries
if (TARGET vtkmdiympi)
  vtkm_diy_set_target_output_directory(vtkmdiympi)
endif()
if (TARGET vtkmdiympi_nompi)
  vtkm_diy_set_target_output_directory(vtkmdiympi_nompi)
endif()

include(VTKmDIYUtils)

add_library(vtkm_diy INTERFACE)
vtkm_diy_init_target()
target_include_directories(vtkm_diy INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}/vtkm/thirdparty/diy>)
target_link_libraries(vtkm_diy INTERFACE vtkmdiy)

# special logic for when both versions of the diy library are built
if (VTKm_ENABLE_DIY_NOMPI)
  # only link vtkmdiympi/vtkmdiympi_nompi when building executable
  set(is_exe "$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>")
  target_link_libraries(vtkm_diy INTERFACE
    "$<LINK_ONLY:$<${is_exe}:$<TARGET_PROPERTY:vtkm_diy,vtkm_diy_target>>>")

  # ignore undefined symbols
  set(is_shared_lib "$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>")
  if (APPLE)
    target_link_libraries(vtkm_diy INTERFACE "$<${is_shared_lib}:-undefined dynamic_lookup>")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    target_link_libraries(vtkm_diy INTERFACE
      "$<${is_shared_lib}:-INCREMENTAL:NO>"
      "$<${is_shared_lib}:-FORCE:UNRESOLVED>")
  endif()
else()
  if (TARGET vtkmdiympi)
    target_link_libraries(vtkm_diy INTERFACE vtkmdiympi)
  else()
    target_link_libraries(vtkm_diy INTERFACE vtkmdiympi_nompi)
  endif()
endif()

#-----------------------------------------------------------------------------
install(TARGETS vtkm_diy EXPORT ${VTKm_EXPORT_NAME})

## Install headers
if (NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(FILES
    ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
    ${CMAKE_CURRENT_SOURCE_DIR}/diy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/environment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mpi-cast.h
    ${CMAKE_CURRENT_SOURCE_DIR}/post-include.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pre-include.h
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization.h
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
