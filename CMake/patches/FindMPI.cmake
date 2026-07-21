##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


# This module is already included in new versions of CMake
if(CMAKE_VERSION VERSION_LESS 3.15)
  include(${CMAKE_CURRENT_LIST_DIR}/3.15/FindMPI.cmake)
else()
  include(${CMAKE_ROOT}/Modules/FindMPI.cmake)
endif()
