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

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(VTKM_COMPILER_IS_GNU 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(VTKM_COMPILER_IS_CLANG 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(VTKM_COMPILER_IS_CLANG 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  set(VTKM_COMPILER_IS_PGI 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(VTKM_COMPILER_IS_ICC 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(VTKM_COMPILER_IS_MSVC 1)
endif()


#-----------------------------------------------------------------------------
# vtkm_compiler_flags is used by all the vtkm targets and consumers of VTK-m
# The flags on vtkm_compiler_flags are needed when using/building vtk-m
add_library(vtkm_compiler_flags INTERFACE)

# When building libraries/tests that are part of the VTK-m repository
# inherit the properties from vtkm_developer_flags and vtkm_vectorization_flags.
# The flags are intended only for VTK-m itself and are not needed by consumers.
# We will export vtkm_vectorization_flags in general so consumer can enable
# vectorization if they so desire
if (VTKm_ENABLE_DEVELOPER_FLAGS)
  target_link_libraries(vtkm_compiler_flags
    INTERFACE $<BUILD_INTERFACE:vtkm_developer_flags>)
endif()
target_link_libraries(vtkm_compiler_flags
  INTERFACE $<BUILD_INTERFACE:vtkm_vectorization_flags>)


# setup that we need C++11 support
if(CMAKE_VERSION VERSION_LESS 3.8)
  target_compile_features(vtkm_compiler_flags INTERFACE cxx_nullptr)
else()
  target_compile_features(vtkm_compiler_flags INTERFACE cxx_std_11)
endif()

# Enable large object support so we can have 2^32 addressable sections
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(vtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/bigobj>)
  if(TARGET vtkm::cuda)
    target_compile_options(vtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler="/bigobj">)
  endif()
endif()

# Setup the include directories that are needed for vtkm
target_include_directories(vtkm_compiler_flags INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}>
  )

#-----------------------------------------------------------------------------
# vtkm_developer_flags is used ONLY BY libraries that are built as part of this
# repository
add_library(vtkm_developer_flags INTERFACE)

# Additional warnings just for Clang 3.5+, and AppleClang 7+
# about failures to vectorize.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.4)
  target_compile_options(vtkm_developer_flags INTERFACE $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:CXX>:-Wno-pass-failed>>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
       CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.99)
  target_compile_options(vtkm_developer_flags INTERFACE $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:CXX>:-Wno-pass-failed>>)
endif()

if(VTKM_COMPILER_IS_MSVC)
  target_compile_definitions(vtkm_developer_flags INTERFACE "_SCL_SECURE_NO_WARNINGS"
                                                            "_CRT_SECURE_NO_WARNINGS")

  #CMake COMPILE_LANGUAGE doesn't work with MSVC, ans since we want these flags
  #only for C++ compilation we have to resort to setting CMAKE_CXX_FLAGS :(
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4702 /wd4505")
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler=\"/wd4702 /wd4505\" -Xcudafe=\"--diag_suppress=1394 --diag_suppress=766 --display_error_number\"")

  if(MSVC_VERSION LESS 1900)
    # In VS2013 the C4127 warning has a bug in the implementation and
    # generates false positive warnings for lots of template code
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4127")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler=\"/wd4127\"")
  endif()

elseif(VTKM_COMPILER_IS_ICC)
  #Intel compiler offers header level suppression in the form of
  # #pragma warning(disable : 1478), but for warning 1478 it seems to not
  #work. Instead we add it as a definition
  # Likewise to suppress failures about being unable to apply vectorization
  # to loops, the #pragma warning(disable seems to not work so we add a
  # a compile define.
  target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-wd1478 -wd13379>)

elseif(VTKM_COMPILER_IS_GNU OR VTKM_COMPILER_IS_CLANG)
  set(cxx_flags -Wall -Wno-long-long -Wcast-align -Wconversion -Wchar-subscripts -Wextra -Wpointer-arith -Wformat -Wformat-security -Wshadow -Wunused-parameter -fno-common)
  set(cuda_flags -Xcudafe=--display_error_number -Xcompiler=-Wall,-Wno-unknown-pragmas,-Wno-unused-local-typedefs,-Wno-unused-local-typedefs,-Wno-unused-function,-Wno-long-long,-Wcast-align,-Wconversion,-Wchar-subscripts,-Wpointer-arith,-Wformat,-Wformat-security,-Wshadow,-Wunused-parameter,-fno-common)

  #GCC 5, 6 don't properly handle strict-overflow suppression through pragma's.
  #Instead of suppressing around the location of the strict-overflow you
  #have to suppress around the entry point, or in vtk-m case the worklet
  #invocation site. This is incredibly tedious and has been fixed in gcc 7
  #
  if(VTKM_COMPILER_IS_GNU AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.99) AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.99) )
    list(APPEND cxx_flags -Wno-strict-overflow)
  endif()
  target_compile_options(vtkm_developer_flags
    INTERFACE $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>>
    )
  if(TARGET vtkm::cuda)
    target_compile_options(vtkm_developer_flags
      INTERFACE $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:CUDA>:${cuda_flags}>>
      )
  endif()
endif()

if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(TARGETS vtkm_compiler_flags vtkm_developer_flags EXPORT ${VTKm_EXPORT_NAME})
endif()
