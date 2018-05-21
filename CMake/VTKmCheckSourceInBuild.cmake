##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2016 UT-Battelle, LLC.
##  Copyright 2016 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

## This CMake script checks to make sure that each source file is explicitly
## listed in the CMakeLists.txt files. This helps ensure that all files that we
## are using are appropriately listed in IDEs and installed as necessary. It
## also helps identify dead files that should no longer be in the repository.
## To run this script, execute CMake as follows:
##
## cmake -DVTKm_SOURCE_DIR=<VTKm_SOURCE_DIR> -P <VTKm_SOURCE_DIR>/CMake/VTKMCheckSourceInBuild.cmake
##

cmake_minimum_required(VERSION 2.8)

set(FILES_TO_CHECK
  *.h
  *.h.in
  *.cxx
  *.cu
  )

set(EXCEPTIONS
  kxsort.h
  )

set(DIRECTORY_EXCEPTIONS
  ${VTKm_SOURCE_DIR}/vtkm/thirdparty/taotuple/vtkmtaotuple
  ${VTKm_SOURCE_DIR}/vtkm/thirdparty/diy/vtkmdiy
  )

if (NOT VTKm_SOURCE_DIR)
  message(SEND_ERROR "VTKm_SOURCE_DIR not defined.")
endif (NOT VTKm_SOURCE_DIR)

function(check_directory directory parent_CMakeLists_contents)
  foreach(exception IN LISTS DIRECTORY_EXCEPTIONS)
    if(directory MATCHES "^${exception}$")
      return()
    endif()
  endforeach(exception)

  message("Checking directory ${directory}...")

  get_filename_component(directory_name "${directory}" NAME)

  if(EXISTS "${directory}/CMakeLists.txt")
    file(READ "${directory}/CMakeLists.txt" CMakeLists_contents)
  endif()

  foreach (glob_expression ${FILES_TO_CHECK})
    file(GLOB file_list
      RELATIVE "${directory}"
      "${directory}/${glob_expression}"
      )

    foreach (file ${file_list})
      set(skip)
      foreach(exception ${EXCEPTIONS})
        if(file MATCHES "^${exception}(/.*)?$")
          # This file is an exception
          set(skip TRUE)
        endif()
      endforeach(exception)

      if(NOT skip)
        message("Checking ${file}")
        # Remove .in suffix. These are generally configured files that generate
        # new files that are actually used in the build.
        string(REGEX REPLACE ".in$" "" file_check "${file}")
        string(FIND "${CMakeLists_contents}" "${file_check}" position)
        if(${position} LESS 0)
          # Check the CMakeLists.txt of the parent directory. Some sources of
          # internal directories are packaged into libraries in the parent
          # directory.
          string(FIND "${parent_CMakeLists_contents}"
            "${directory_name}/${file_check}"
            position
            )
          if(${position} LESS 0)
            message(SEND_ERROR
              "****************************************************************
${file_check} is not found in ${directory}/CMakeLists.txt
This indicates that the file is not part of the build system. Thus it might be missing build targets. All such files should be explicitly handled by CMake.")
          endif() # Not in parent's CMakeLists.txt
        endif() # Not in CMakeLists.txt
      endif() # Not skipped
    endforeach (file)
  endforeach(glob_expression)

  file(GLOB file_list
    LIST_DIRECTORIES true
    "${directory}/*")
  foreach(file ${file_list})
    if(IS_DIRECTORY "${file}")
      check_directory("${file}" "${CMakeLists_contents}")
    endif()
  endforeach(file)
endfunction(check_directory)

check_directory("${VTKm_SOURCE_DIR}/vtkm" "")
check_directory("${VTKm_SOURCE_DIR}/examples" "")
