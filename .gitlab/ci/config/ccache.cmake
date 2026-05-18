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

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

set(version 4.13.6)
set(base_url https://github.com/ccache/ccache/releases/download)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  set(sha256sum 508b2a1217dc6e04a23e967c7b95a0fb45d8a7e16fde9e180919698f2e2be060)
  set(filename "ccache-${version}-linux-x86_64-glibc")
  set(extension tar.xz)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  set(sha256sum 0274210ec9c9936ed5711d59b0de3167a51216a588ddde35f6bc828f366fe6d9)
  set(filename "ccache-${version}-darwin")
  set(extension tar.gz)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
  set(sha256sum 3d7cebb05850ad704e197b3f1d3f0f924ab6c9fdfc561578e146184fe9d89380)
  set(filename "ccache-${version}-windows-x86_64")
  set(extension zip)
else()
  message(FATAL_ERROR "Unrecognized platform ${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(tarball "${filename}.${extension}")
set(full_url "${base_url}/v${version}/${tarball}")

file(DOWNLOAD
  "${full_url}" $ENV{CCACHE_INSTALL_DIR}/${tarball}
  EXPECTED_HASH SHA256=${sha256sum}
  SHOW_PROGRESS
  )

execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar xf ${tarball} ${filename}/ccache
  WORKING_DIRECTORY $ENV{CCACHE_INSTALL_DIR}
  RESULT_VARIABLE extract_results
  )

if(extract_results)
  message(FATAL_ERROR "Extracting `${tarball}` failed: ${extract_results}.")
endif()

file(MAKE_DIRECTORY $ENV{CCACHE_INSTALL_DIR}/ccache)
file(RENAME $ENV{CCACHE_INSTALL_DIR}/${filename}/ccache $ENV{CCACHE_INSTALL_DIR}/ccache/ccache)
