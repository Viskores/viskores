##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

if (NOT (DEFINED VTKm_BUILD_CMAKE_BASE_DIR AND
         DEFINED VTKm_INSTALL_CONFIG_DIR AND
         DEFINED VTKm_CMAKE_MODULE_PATH))
  message(FATAL_ERROR
    "VTKmInstallCMakePackage is missing input variables")
endif()

set(vtkm_cmake_module_files)

if(VTKm_ENABLE_TBB)
  list(APPEND vtkm_cmake_module_files FindTBB.cmake)
endif()

set(vtkm_cmake_build_dir ${VTKm_BUILD_CMAKE_BASE_DIR}/${VTKm_INSTALL_CONFIG_DIR})
foreach (vtkm_cmake_module_file IN LISTS vtkm_cmake_module_files)
  configure_file(
    "${VTKm_CMAKE_MODULE_PATH}/${vtkm_cmake_module_file}"
    "${vtkm_cmake_build_dir}/${vtkm_cmake_module_file}"
    COPYONLY)
  list(APPEND vtkm_cmake_files_to_install
    "${vtkm_cmake_module_file}")
endforeach()

foreach (vtkm_cmake_file IN LISTS vtkm_cmake_files_to_install)
  if (IS_ABSOLUTE "${vtkm_cmake_file}")
    file(RELATIVE_PATH vtkm_cmake_subdir_root "${vtkm_cmake_build_dir}" "${vtkm_cmake_file}")
    get_filename_component(vtkm_cmake_subdir "${vtkm_cmake_subdir_root}" DIRECTORY)
    set(vtkm_cmake_original_file "${vtkm_cmake_file}")
  else ()
    get_filename_component(vtkm_cmake_subdir "${vtkm_cmake_file}" DIRECTORY)
    set(vtkm_cmake_original_file "${VTKm_CMAKE_MODULE_PATH}/${vtkm_cmake_file}")
  endif ()
  install(
    FILES       "${vtkm_cmake_original_file}"
    DESTINATION "${VTKm_INSTALL_CONFIG_DIR}/${vtkm_cmake_subdir}"
    COMPONENT   "development")
endforeach ()
