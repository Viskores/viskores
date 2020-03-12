##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
   CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  set(VTKM_COMPILER_IS_MSVC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  set(VTKM_COMPILER_IS_PGI 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(VTKM_COMPILER_IS_ICC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(VTKM_COMPILER_IS_CLANG 1)
  set(VTKM_COMPILER_IS_APPLECLANG 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(VTKM_COMPILER_IS_CLANG 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(VTKM_COMPILER_IS_GNU 1)
endif()

#-----------------------------------------------------------------------------
# vtkm_compiler_flags is used by all the vtkm targets and consumers of VTK-m
# The flags on vtkm_compiler_flags are needed when using/building vtk-m
add_library(vtkm_compiler_flags INTERFACE)

# When building libraries/tests that are part of the VTK-m repository
# inherit the properties from vtkm_vectorization_flags.
# The flags are intended only for VTK-m itself and are not needed by consumers.
# We will export vtkm_vectorization_flags in general
# so consumer can either enable vectorization or use VTK-m's build flags if
# they so desire
target_link_libraries(vtkm_compiler_flags
  INTERFACE $<BUILD_INTERFACE:vtkm_vectorization_flags>)

# setup that we need C++11 support
target_compile_features(vtkm_compiler_flags INTERFACE cxx_std_11)

# setup our static libraries so that a separate ELF section
# is generated for each function. This allows for the linker to
# remove unused sections. This allows for programs that use VTK-m
# to have the smallest binary impact as they can drop any VTK-m symbol
# they don't use.
if(VTKM_COMPILER_IS_MSVC)
  target_compile_options(vtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/Gy>)
  if(TARGET vtkm::cuda)
    target_compile_options(vtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler="/Gy">)
  endif()
elseif(NOT VTKM_COMPILER_IS_PGI) #can't find an equivalant PGI flag
  target_compile_options(vtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>)
  if(TARGET vtkm::cuda)
    target_compile_options(vtkm_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-ffunction-sections>)
  endif()
endif()

# Enable large object support so we can have 2^32 addressable sections
if(VTKM_COMPILER_IS_MSVC)
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
if (VTKM_COMPILER_IS_CLANG AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.4)
  target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-Wno-pass-failed>)
elseif(VTKM_COMPILER_IS_APPLECLANG AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.99)
  target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-Wno-pass-failed>)
endif()

if(VTKM_COMPILER_IS_MSVC)
  target_compile_definitions(vtkm_developer_flags INTERFACE "_SCL_SECURE_NO_WARNINGS"
                                                            "_CRT_SECURE_NO_WARNINGS")

  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
    set(cxx_flags "-W3")
    set(cuda_flags "-Xcompiler=-W3")
  endif()
  list(APPEND cxx_flags -wd4702 -wd4505)
  list(APPEND cuda_flags "-Xcompiler=-wd4702,-wd4505")

  #Setup MSVC warnings with CUDA and CXX
  target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>)
  if(TARGET vtkm::cuda)
    target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:${cuda_flags}  -Xcudafe=--diag_suppress=1394,--diag_suppress=766>)
  endif()

  if(MSVC_VERSION LESS 1900)
    # In VS2013 the C4127 warning has a bug in the implementation and
    # generates false positive warnings for lots of template code
    target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-wd4127>)
    if(TARGET vtkm::cuda)
      target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-wd4127>)
    endif()
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
  set(cxx_flags -Wall -Wcast-align -Wchar-subscripts -Wextra -Wpointer-arith -Wformat -Wformat-security -Wshadow -Wunused -fno-common)
  set(cuda_flags -Xcompiler=-Wall,-Wno-unknown-pragmas,-Wno-unused-local-typedefs,-Wno-unused-local-typedefs,-Wno-unused-function,-Wcast-align,-Wchar-subscripts,-Wpointer-arith,-Wformat,-Wformat-security,-Wshadow,-Wunused,-fno-common)

  #Only add float-conversion warnings for gcc as the integer warnigns in GCC
  #include the implicit casting of all types smaller than int to ints.
  if (VTKM_COMPILER_IS_GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.99)
    list(APPEND cxx_flags -Wfloat-conversion)
    set(cuda_flags "${cuda_flags},-Wfloat-conversion")
  elseif (VTKM_COMPILER_IS_CLANG)
    list(APPEND cxx_flags -Wconversion)
    set(cuda_flags "${cuda_flags},-Wconversion")
  endif()

  #Add in the -Wodr warning for GCC versions 5.2+
  if (VTKM_COMPILER_IS_GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.1)
    list(APPEND cxx_flags -Wodr)
    set(cuda_flags "${cuda_flags},-Wodr")
  elseif (VTKM_COMPILER_IS_CLANG)
    list(APPEND cxx_flags -Wodr)
    set(cuda_flags "${cuda_flags},-Wodr")
  endif()

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

  target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>)
  if(TARGET vtkm::cuda)
    target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:${cuda_flags}>)
  endif()
endif()

#common warnings for all platforms when building cuda
if(TARGET vtkm::cuda)
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
    #nvcc 9 introduced specific controls to disable the stack size warning
    #otherwise we let the warning occur. We have to set this in CMAKE_CUDA_FLAGS
    #as it is passed to the device link step, unlike compile_options
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xnvlink=--suppress-stack-size-warning")
  endif()

  set(display_error_nums -Xcudafe=--display_error_number)
  target_compile_options(vtkm_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:${display_error_nums}>)
endif()

if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(TARGETS vtkm_compiler_flags vtkm_developer_flags EXPORT ${VTKm_EXPORT_NAME})
endif()
