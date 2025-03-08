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

file(GLOB cmake_version_backports
  LIST_DIRECTORIES true
  RELATIVE "${CMAKE_CURRENT_LIST_DIR}/patches"
  "${CMAKE_CURRENT_LIST_DIR}/patches/*")

foreach (cmake_version_backport IN LISTS cmake_version_backports)
  if (NOT IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/patches/${cmake_version_backport}")
    continue ()
  endif ()
  if (CMAKE_VERSION VERSION_LESS "${cmake_version_backport}")
    list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}/patches/${cmake_version_backport}")
  endif ()
endforeach ()
