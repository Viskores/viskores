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

## This CMake script checks to make sure that each source file explicitly
## inside a project is installed.
## To run this script, execute CMake as follows:
##
## cmake -DMODE=[INSTALL|VERIFY|CLEANUP]
#        -DViskores_SOURCE_DIR=<Viskores_SOURCE_DIR>
#        -DViskores_BINARY_DIR=<Viskores_BINARY_DIR>
#        -DViskores_INSTALL_INCLUDE_DIR=<Viskores_INSTALL_INCLUDE_DIR>
#        -DDIR_EXCEPTIONS=<dir>:<dir>:...
#        -DViskores_ENABLE_HDF5_IO=<Viskores_ENABLE_HDF5_IO>
#        -P <Viskores_SOURCE_DIR>/CMake/testing/VISKORESCheckSourceInInstall.cmake
##

if (NOT DEFINED MODE)
  message(FATAL_ERROR "Need to pass the MODE variable (INSTALL|VERIFY|CLEANUP) so the script knows what to do")
endif ()
if (NOT Viskores_SOURCE_DIR)
  message(FATAL_ERROR "Viskores_SOURCE_DIR not defined.")
endif ()
if (NOT Viskores_BINARY_DIR)
  message(FATAL_ERROR "Viskores_BINARY_DIR not defined.")
endif ()
if (NOT Viskores_INSTALL_INCLUDE_DIR)
  message(FATAL_ERROR "Viskores_INSTALL_INCLUDE_DIR not defined.")
endif ()
if (NOT DEFINED DIR_EXCEPTIONS)
  message(FATAL_ERROR "DIR_EXCEPTIONS not defined.")
endif()
if (NOT DEFINED Viskores_ENABLE_HDF5_IO)
  message(FATAL_ERROR "Viskores_ENABLE_HDF5_IO not defined.")
endif()

include(CMakeParseArguments)
# -----------------------------------------------------------------------------
function(verify_install_per_dir src_directory build_dir)
  set(options )
  set(oneValueArgs )
  set(multiValueArgs EXTENSIONS FILE_EXCEPTIONS DIR_EXCEPTIONS)
  cmake_parse_arguments(verify
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  set(files_to_verify )
  foreach(ext IN LISTS verify_EXTENSIONS)
    file(GLOB_RECURSE listing
         RELATIVE "${src_directory}"
         "${src_directory}/${ext}"
         )
    list(APPEND files_to_verify ${listing})
  endforeach()

  #remove all files that are exempt
  list(REMOVE_ITEM files_to_verify ${verify_FILE_EXCEPTIONS})

  #remove all files inside directories that match
  foreach(dir IN LISTS verify_DIR_EXCEPTIONS)
    list(FILTER files_to_verify EXCLUDE REGEX "^${dir}")
  endforeach()

  set(to_fail FALSE) # error out after listing all missing headers
  foreach(file IN LISTS files_to_verify)
    if(NOT EXISTS ${build_dir}/${file})
      message(STATUS "file: ${file} not installed \n\tWas expecting it to be at: ${build_dir}/${file}")
      set(to_fail TRUE)
    # else()
    #   message(STATUS "file: ${file} installed")
    endif()
  endforeach()

  if(to_fail)
    message(FATAL_ERROR "unable to find all headers in the install tree")
  endif()
endfunction()

# -----------------------------------------------------------------------------
function(do_install root_dir prefix)
  #Step 1. Setup up our new install prefix location
  set(CMAKE_INSTALL_PREFIX  ${root_dir}/${prefix})
  set(CMAKE_INSTALL_COMPONENT FALSE)

  #Step 2. Execute the install command  files
  if(EXISTS "${root_dir}/cmake_install.cmake")
    include(${root_dir}/cmake_install.cmake)
  else()
    message(FATAL_ERROR "Viskores looks to have no install rules as we can't find any cmake_install.cmake files in the root of the build directory.")
  endif()

endfunction()

# -----------------------------------------------------------------------------
function(do_verify root_dir prefix)
  #Step 1. Setup the extensions to check, and all file and directory
  # extensions
  set(files_extensions
    *.hpp #needed for diy
    *.h
    *.hxx
    )

  set(file_exceptions
    thirdparty/diy/viskoresdiy/cmake/mpi_types.h
    thirdparty/lodepng/viskoreslodepng/lodepng.h
    thirdparty/loguru/viskoresloguru/loguru.hpp
    )

  string(REPLACE ":" ";" directory_exceptions "${DIR_EXCEPTIONS}")
  #by default every header in a testing directory doesn't need to be installed
  list(APPEND directory_exceptions ".*/testing")
  # These exceptions should be based on the status of the associated
  # cmake option
  if (NOT Viskores_ENABLE_HDF5_IO)
    list(APPEND file_exceptions
      io/ImageWriterHDF5.h
      io/ImageReaderHDF5.h
      )
  endif()
  #Step 2. Verify the installed files match what headers are listed in each
  # source directory
  verify_install_per_dir("${Viskores_SOURCE_DIR}/viskores"
                         "${root_dir}/${prefix}/${Viskores_INSTALL_INCLUDE_DIR}/viskores"
                         EXTENSIONS ${files_extensions}
                         FILE_EXCEPTIONS ${file_exceptions}
                         DIR_EXCEPTIONS ${directory_exceptions}
                         )
endfunction()

# -----------------------------------------------------------------------------
function(do_cleanup root_dir prefix)
  #Step 1. Remove temp directory
  file(REMOVE_RECURSE "${root_dir}/${prefix}")
endfunction()

set(root_dir "${Viskores_BINARY_DIR}")
set(prefix "/CMakeFiles/_tmp_install")

message(STATUS "MODE: ${MODE}")
if(MODE STREQUAL "INSTALL")
  do_install(${root_dir} ${prefix})
elseif(MODE STREQUAL "VERIFY")
  do_verify(${root_dir} ${prefix})
elseif(MODE STREQUAL "CLEANUP")
  do_cleanup(${root_dir} ${prefix})
endif()

unset(prefix)
unset(root_dir)
