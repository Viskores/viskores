##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2016 Sandia Corporation.
##  Copyright 2016 UT-Battelle, LLC.
##  Copyright 2016 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
  )

if (NOT VTKm_SOURCE_DIR)
  message(SEND_ERROR "VTKm_SOURCE_DIR not defined.")
endif (NOT VTKm_SOURCE_DIR)

function(check_directory directory)
  message("Checking directory ${directory}...")

  if(EXISTS "${directory}/CMakeLists.txt")
    file(READ "${directory}/CMakeLists.txt" CMakeListsContents)
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
        string(FIND "${CMakeListsContents}" "${file_check}" position)
        if(${position} LESS 0)
          message(SEND_ERROR
            "****************************************************************
${file_check} is not found in ${directory}/CMakeLists.txt
This indicates that the file is not part of the build system. Thus it might be missing build targets. All such files should be explicitly handled by CMake.")
        endif()
      endif()
    endforeach (file)
  endforeach(glob_expression)

  file(GLOB file_list
    LIST_DIRECTORIES true
    "${directory}/*")
  foreach(file ${file_list})
    if(IS_DIRECTORY "${file}")
      check_directory("${file}")
    endif()
  endforeach(file)
endfunction(check_directory)

check_directory("${VTKm_SOURCE_DIR}/vtkm")
check_directory("${VTKm_SOURCE_DIR}/examples")
