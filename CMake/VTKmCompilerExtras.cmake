##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  set(CMAKE_COMPILER_IS_PGIXX 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(CMAKE_COMPILER_IS_ICCXX 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_COMPILER_IS_MSVCXX 1)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)

  include(CheckCXXCompilerFlag)

  # Standard warning flags we should always have
  set(CMAKE_CXX_FLAGS_WARN " -Wall")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
    "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_FLAGS_WARN}")
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_WARN}")

  # Additional warnings for GCC/Clang
  set(CMAKE_CXX_FLAGS_WARN_EXTRA "-Wno-long-long -Wcast-align -Wconversion -Wchar-subscripts -Wextra -Wpointer-arith -Wformat -Wformat-security -Wshadow -Wunused-parameter -fno-common")

  # Additional warnings just for Clang 3.5+, and AppleClang 7+ we specify
  # for all build types, since these failures to vectorize are not limited
  # to debug builds
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
      CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.4)
    set(CMAKE_CXX_FLAGS "-Wno-pass-failed ${CMAKE_CXX_FLAGS}")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
         CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.99)
    set(CMAKE_CXX_FLAGS "-Wno-pass-failed ${CMAKE_CXX_FLAGS}")
  endif()

  # Set up the debug CXX_FLAGS for extra warnings
  option(VTKm_EXTRA_COMPILER_WARNINGS "Add compiler flags to do stricter checking when building debug." ON)
  # We used to add the compiler flags globally, but this caused problems with
  # the CUDA compiler (and its lack of support for GCC pragmas).  Instead,
  # the vtkm_declare_headers and vtkm_unit_tests CMake functions add these flags
  # to their compiles.  As long as the unit tests have good coverage, this
  # should catch all problems.
  if(VTKm_EXTRA_COMPILER_WARNINGS)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_FLAGS_WARN_EXTRA}")
    set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_WARN_EXTRA}")
  endif()
elseif(CMAKE_COMPILER_IS_ICCXX)
  #Intel compiler offers header level suppression in the form of
  # #pragma warning(disable : 1478), but for warning 1478 it seems to not
  #work. Instead we add it as a definition
  add_definitions("-wd1478")
  # Likewise to suppress failures about being unable to apply vectorization
  # to loops, the #pragma warning(disable seems to not work so we add a
  # a compile define.
  add_definitions("-wd13379")
elseif (CMAKE_COMPILER_IS_MSVCXX)
  #enable large object support so we can have 2^32 addressable sections
  list(APPEND VTKm_COMPILE_OPTIONS "/bigobj")

  # Use the highest warning level for visual c++ compiler.
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()

endif()
