##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

#Currently all we are going to build is a set of options that are possible
#based on the compiler. For now we are going on the presumption
#that x86 architecture is the only target for vectorization and therefore
#we don't need any system detect.
#
#Here is the breakdown of what each flag type means:
#
#  1. none:
#  Do not explicitly enable vectorization, but at the same don't explicitly disable
#  vectorization.
#
#  2. native:
#  Allow the compiler to use auto-detection based on the systems CPU to determine
#  the highest level of vectorization support that is allowed. This means that
#  libraries and executables built with this setting are non-portable.
#
#  3. avx
#  Compile with just AVX enabled, no AVX2 or AVX512 vectorization will be used.
#  This means that Sandy Bridge, Ivy Bridge, Haswell, and Skylake are supported,
#  but Haswell and newer will not use any AVX2 instructions
#
#  4. avx2
#  Compile with  AVX2/AVX enabled, no AVX512 vectorization will be used.
#  This means that Sandy Bridge, and Ivy Bridge can not run the code.
#
#  5. avx512
#  Compile with AVX512/AVX2/AVX options enabled.
#  This means that Sandy Bridge, Ivy Bridge, Haswell and can not run the code.
#  Only XeonPhi Knights Landing and Skylake processors can run the code.
#
#  AVX512 is designed to mix with avx/avx2 without any performance penalties,
#  so we enable AVX2 so that we get AVX2 support for < 32bit value types which
#  AVX512 has less support for
#
#
# I wonder if we should go towards a per platform cmake include that stores
# all this knowledge
#   include(gcc.cmake)
#   include(icc.cmake)
#   include(clang.cmake)
#
# This way we could also do compile warning flag detection at the same time
#
#
# Note: By default we use 'native' as the default option
#
#
set(vec_levels none native)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  #for now we presume gcc > 4.6
  list(APPEND vec_levels avx)

  #common flags for the avx instructions for the gcc compiler
  set_property(GLOBAL PROPERTY VTKm_NATIVE_FLAGS -march=native)
  set_property(GLOBAL PROPERTY VTKm_AVX_FLAGS -mavx)
  set_property(GLOBAL PROPERTY VTKm_AVX2_FLAGS -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2)

  if (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.7 OR
      CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7)
    #if GNU is less than 4.9 you get avx, avx2
    list(APPEND vec_levels avx2)
  elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.1)
    #if GNU is less than 5.1 you get avx, avx2, and some avx512
    list(APPEND vec_levels avx2 avx512)
    set_property(GLOBAL PROPERTY VTKm_AVX512_FLAGS -mavx512f -mavx512pf -mavx512er -mavx512cd)
  else()
    #if GNU is 5.1+ you get avx, avx2, and more avx512
    list(APPEND vec_levels avx2 avx512)
    set_property(GLOBAL PROPERTY VTKm_AVX512_FLAGS -mavx512f -mavx512pf -mavx512er -mavx512cd -mavx512vl -mavx512bw -mavx512dq -mavx512ifma -mavx512vbmi)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND vec_levels avx avx2 avx512)
  set_property(GLOBAL PROPERTY VTKm_NATIVE_FLAGS -march=native)
  set_property(GLOBAL PROPERTY VTKm_AVX_FLAGS -mavx)
  set_property(GLOBAL PROPERTY VTKm_AVX2_FLAGS -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2)
  set_property(GLOBAL PROPERTY VTKm_AVX512_FLAGS -mavx512)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  #While Clang support AVX512, no version of AppleClang has that support yet
  list(APPEND vec_levels avx avx2)
  set_property(GLOBAL PROPERTY VTKm_NATIVE_FLAGS -march=native)
  set_property(GLOBAL PROPERTY VTKm_AVX_FLAGS -mavx)
  set_property(GLOBAL PROPERTY VTKm_AVX2_FLAGS -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  #I can't find documentation to explicitly state the level of vectorization
  #support I want from the PGI compiler
  #so for now we are going to do nothing
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  #Intel 15.X is the first version with avx512
  #Intel 16.X has way better vector generation compared to 15.X though

  set_property(GLOBAL PROPERTY VTKm_NATIVE_FLAGS -xHost)
  set_property(GLOBAL PROPERTY VTKm_AVX_FLAGS  -xAVX)
  set_property(GLOBAL PROPERTY VTKm_AVX2_FLAGS -xCORE-AVX2)

  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15.0)
    list(APPEND vec_levels avx avx2)
  else()
    list(APPEND vec_levels avx avx2 avx512)
    set_property(GLOBAL PROPERTY VTKm_AVX2_FLAGS -xCORE-AVX512)
  endif()
endif()

#
# Now that we have set up what levels the compiler lets setup the CMake option
# We use a combo box style property, so that ccmake and cmake-gui have a
# nice interface
#
set(VTKm_Vectorization "native" CACHE STRING "Level of compiler vectorization support")
set_property(CACHE VTKm_Vectorization PROPERTY STRINGS ${vec_levels})

#
# Now that we have set up the options, lets setup the compile flags that
# we are going to require.
#
set(flags)
if(VTKm_Vectorization STREQUAL "native")
  get_property(flags GLOBAL PROPERTY VTKm_NATIVE_FLAGS)
elseif(VTKm_Vectorization STREQUAL "avx")
  get_property(flags GLOBAL PROPERTY VTKm_AVX_FLAGS)
elseif(VTKm_Vectorization STREQUAL "avx2")
  get_property(avx GLOBAL PROPERTY VTKm_AVX_FLAGS)
  get_property(avx2 GLOBAL PROPERTY VTKm_AVX2_FLAGS)
  set(flags ${avx} ${avx2})
elseif(VTKm_Vectorization STREQUAL "avx512")
  get_property(avx GLOBAL PROPERTY VTKm_AVX_FLAGS)
  get_property(avx2 GLOBAL PROPERTY VTKm_AVX2_FLAGS)
  get_property(avx512 GLOBAL PROPERTY VTKm_AVX512_FLAGS)
  set(flags ${avx} ${avx2} ${avx512})
endif()

#have to specify each compile option separately, can't do them in bulk
foreach(flag ${flags})
  add_compile_options( ${flag} )
endforeach()
