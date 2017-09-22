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

# This file provides all the component-specific configuration. To add a
# component to this list, first add the name of the component to
# VTKm_AVAILABLE_COMPONENTS. Then add a macro (or function) named
# vtkm_configure_component_<name> that configures the given component. At a
# minimum, this macro should set (or clear) the variable VTKm_<name>_FOUND. It
# should also modify other apropos variables such as VTKm_INCLUDE_DIRS and
# VTKm_LIBRARIES.
#
# Components generally rely on other components, and should call
# vtkm_configure_component_<name> for those components as necessary.
#
# Any component configuration added to VTKm_AVAILABLE_COMPONENTS will
# automatically be added to the components available to find_package(VTKm).
#

set(VTKm_AVAILABLE_COMPONENTS
  Base
  Serial
  OpenGL
  OSMesa
  EGL
  GLFW
  GLUT
  Rendering
  TBB
  CUDA
  )

#-----------------------------------------------------------------------------
# Support function for giving status messages on component configurations
#-----------------------------------------------------------------------------
set(VTKm_CONFIGURE_COMPONENT_MESSAGES "" CACHE INTERNAL "" FORCE)
function(vtkm_configure_component_message message_text)
  if(NOT VTKm_CONFIGURE_QUIET)
    list(FIND VTKm_CONFIGURE_COMPONENT_MESSAGES "${message_text}" in_list)
    if(in_list EQUAL -1)
      message(STATUS "${message_text}")
      set(VTKm_CONFIGURE_COMPONENT_MESSAGES "${VTKm_CONFIGURE_COMPONENT_MESSAGES} ${message_text}"
          CACHE STRING "" FORCE)
    endif()
  endif()
endfunction(vtkm_configure_component_message)

#-----------------------------------------------------------------------------
# Support function for making vtkm_configure_component<name> functions.
#-----------------------------------------------------------------------------
macro(vtkm_finish_configure_component component)
  if(NOT VTKm_${component}_FOUND)

    cmake_parse_arguments(VTKm_FCC
      "IS_BACKEND"
      ""
      "DEPENDENT_VARIABLES;ADD_INCLUDES;ADD_LIBRARIES"
      ${ARGN}
      )
    set(VTKm_${component}_FOUND TRUE)
    foreach(var ${VTKm_FCC_DEPENDENT_VARIABLES})
      if(NOT ${var})
        set(VTKm_${component}_FOUND)
        vtkm_configure_component_message(
          "Failed to configure VTK-m component ${component}: !${var}")
        break()
      endif()
    endforeach(var)

    if (VTKm_${component}_FOUND)
      set(VTKm_INCLUDE_DIRS ${VTKm_INCLUDE_DIRS} ${VTKm_FCC_ADD_INCLUDES})
      set(VTKm_LIBRARIES ${VTKm_LIBRARIES} ${VTKm_FCC_ADD_LIBRARIES})
      if(${VTKm_FCC_IS_BACKEND})
        set(VTKm_BACKEND_INCLUDE_DIRS ${VTKm_BACKEND_INCLUDE_DIRS} ${VTKm_FCC_ADD_INCLUDES})
        set(VTKm_BACKEND_LIBRARIES ${VTKm_BACKEND_LIBRARIES} ${VTKm_FCC_ADD_LIBRARIES})
      endif()
    endif()
  endif()
endmacro()

#-----------------------------------------------------------------------------
# The configuration macros
#-----------------------------------------------------------------------------

macro(vtkm_configure_component_Base)
  # Set up the compiler flag optimizations
  if (NOT VTKm_Vectorization_flags_added)
    include(VTKmCompilerOptimizations)
  endif()

  # Check for the existance of the base vtkm target
  if (TARGET vtkm)
    set(VTKm_base_vtkm_target_FOUND True)
  endif()

  vtkm_finish_configure_component(Base
    DEPENDENT_VARIABLES VTKm_base_vtkm_target_FOUND
    ADD_LIBRARIES vtkm vtkm_cont
    )
endmacro()

macro(vtkm_configure_component_Serial)
  vtkm_configure_component_Base()

  vtkm_finish_configure_component(Serial
    IS_BACKEND
    DEPENDENT_VARIABLES VTKm_Base_FOUND
    )
endmacro(vtkm_configure_component_Serial)

macro(vtkm_configure_component_OpenGL)
  # OpenGL configuration "depends" on OSMesa because if OSMesa is used, then it
  # (sometimes) requires its own version of OpenGL. The find_package for OpenGL
  # is smart enough to configure this correctly if OSMesa is found first. Thus,
  # we ensure that OSMesa is configured before OpenGL (assuming you are using
  # the VTK-m configuration). However, the OpenGL configuration can still
  # succeed even if the OSMesa configuration fails.
  vtkm_configure_component_OSMesa()

  if(NOT VTKm_OSMesa_FOUND)
    find_package(OpenGL ${VTKm_FIND_PACKAGE_QUIETLY})

    set(vtkm_opengl_dependent_vars VTKm_Base_FOUND OPENGL_FOUND)
    set(vtkm_opengl_includes ${OPENGL_INCLUDE_DIR})
    set(vtkm_opengl_libraries ${OPENGL_LIBRARIES})
  else()
    # OSMesa comes with its own implementation of OpenGL. So if OSMesa has been
    # found, then simply report that OpenGL has been found and use the includes
    # and libraries already added for OSMesa.
    set(vtkm_opengl_dependent_vars)
    set(vtkm_opengl_includes)
    set(vtkm_opengl_libraries)
  endif()

  # Many OpenGL classes in VTK-m require GLEW (too many to try to separate them
  # out and still get something worth using). So require that too.
  find_package(GLEW ${VTKm_FIND_PACKAGE_QUIETLY})

  list(APPEND vtkm_opengl_dependent_vars GLEW_FOUND)
  if(GLEW_FOUND)
    list(APPEND vtkm_opengl_includes ${GLEW_INCLUDE_DIRS})
    list(APPEND vtkm_opengl_libraries ${GLEW_LIBRARIES})
  endif()
  #on unix/linux Glew uses pthreads, so we need to find that, and link to it
  #explicitly or else in release mode we get sigsegv on launch
  if(UNIX)
    find_package(Threads ${VTKm_FIND_PACKAGE_QUIETLY})
    list(APPEND vtkm_opengl_libraries ${CMAKE_THREAD_LIBS_INIT})
  endif()

  vtkm_finish_configure_component(OpenGL
    DEPENDENT_VARIABLES ${vtkm_opengl_dependent_vars}
    ADD_INCLUDES ${vtkm_opengl_includes}
    ADD_LIBRARIES ${vtkm_opengl_libraries}
    )

  #setting VTKm_OPENGL_INCLUDE_DIRS when both mesa and
  #opengl are not present causes cmake to fail to configure
  #becase of a percieved dependency in the rendering lib
  if(VTKm_OSMesa_FOUND OR OPENGL_FOUND)
    set(VTKm_OPENGL_INCLUDE_DIRS ${vtkm_opengl_includes})
    set(VTKm_OPENGL_LIBRARIES  ${vtkm_opengl_libraries})
  endif()

endmacro(vtkm_configure_component_OpenGL)

macro(vtkm_configure_component_OSMesa)
  vtkm_configure_component_Base()

  if (VTKm_ENABLE_OSMESA)
    find_package(MESA ${VTKm_FIND_PACKAGE_QUIETLY})

    vtkm_finish_configure_component(OSMesa
      DEPENDENT_VARIABLES OSMESA_FOUND
      ADD_INCLUDES ${OSMESA_INCLUDE_DIR}
      ADD_LIBRARIES ${OSMESA_LIBRARY}
      )
  endif()
endmacro(vtkm_configure_component_OSMesa)

macro(vtkm_configure_component_EGL)
  vtkm_configure_component_OpenGL()

  find_package(EGL ${VTKm_FIND_PACKAGE_QUIETLY})

  vtkm_finish_configure_component(EGL
    DEPENDENT_VARIABLES VTKm_OpenGL_FOUND EGL_FOUND
    ADD_INCLUDES ${EGL_INCLUDE_DIRS}
    ADD_LIBRARIES ${EGL_LIBRARIES}
    )
endmacro(vtkm_configure_component_EGL)

macro(vtkm_configure_component_GLFW)
  vtkm_configure_component_OpenGL()

  find_package(GLFW ${VTKm_FIND_PACKAGE_QUIETLY})

  vtkm_finish_configure_component(GLFW
    DEPENDENT_VARIABLES VTKm_OpenGL_FOUND GLFW_FOUND
    ADD_INCLUDES ${GLFW_INCLUDE_DIRS}
    ADD_LIBRARIES ${GLFW_LIBRARIES}
    )
endmacro(vtkm_configure_component_GLFW)

macro(vtkm_configure_component_GLUT)
  vtkm_configure_component_OpenGL()

  find_package(GLUT ${VTKm_FIND_PACKAGE_QUIETLY})

  vtkm_finish_configure_component(GLUT
    DEPENDENT_VARIABLES VTKm_OpenGL_FOUND GLUT_FOUND
    ADD_INCLUDES ${GLUT_INCLUDE_DIR}
    ADD_LIBRARIES ${GLUT_LIBRARIES}
    )
endmacro(vtkm_configure_component_GLUT)

macro(vtkm_configure_component_Rendering)
  if(VTKm_ENABLE_RENDERING)
    vtkm_configure_component_OpenGL()
    vtkm_configure_component_EGL()
    vtkm_configure_component_OSMesa()
  endif()

  vtkm_finish_configure_component(Rendering
    DEPENDENT_VARIABLES VTKm_ENABLE_RENDERING VTKm_Base_FOUND
    ADD_LIBRARIES vtkm_rendering
    )
endmacro(vtkm_configure_component_Rendering)

macro(vtkm_configure_component_TBB)
  if(VTKm_ENABLE_TBB)
    vtkm_configure_component_Base()

    find_package(TBB ${VTKm_FIND_PACKAGE_QUIETLY})
  endif()

  vtkm_finish_configure_component(TBB
    IS_BACKEND
    DEPENDENT_VARIABLES VTKm_ENABLE_TBB VTKm_Base_FOUND TBB_FOUND
    ADD_INCLUDES ${TBB_INCLUDE_DIRS}
    ADD_LIBRARIES ${TBB_LIBRARIES}
    )
endmacro(vtkm_configure_component_TBB)

macro(vtkm_configure_component_CUDA)
  if(VTKm_ENABLE_CUDA)
    vtkm_configure_component_Base()

    find_package(CUDA ${VTKm_FIND_PACKAGE_QUIETLY})

    #Make cuda link privately to cuda libraries
    set(CUDA_LIBRARIES PRIVATE ${CUDA_LIBRARIES})

    mark_as_advanced(
      CUDA_BUILD_CUBIN
      CUDA_BUILD_EMULATION
      CUDA_HOST_COMPILER
      CUDA_SDK_ROOT_DIR
      CUDA_SEPARABLE_COMPILATION
      CUDA_TOOLKIT_ROOT_DIR
      CUDA_VERBOSE_BUILD
      )

    find_package(Thrust ${VTKm_FIND_PACKAGE_QUIETLY})
  endif()

  vtkm_finish_configure_component(CUDA
    IS_BACKEND
    DEPENDENT_VARIABLES
      VTKm_ENABLE_CUDA
      VTKm_Base_FOUND
      CUDA_FOUND
      THRUST_FOUND
    ADD_INCLUDES ${THRUST_INCLUDE_DIRS}
    )

  if(VTKm_CUDA_FOUND)
    #---------------------------------------------------------------------------
    # Setup build flags for CUDA to have C++11 support
    #---------------------------------------------------------------------------
    if(NOT MSVC)
      if(NOT "--std" IN_LIST CUDA_NVCC_FLAGS)
        list(APPEND CUDA_NVCC_FLAGS --std c++11)
      endif()
    endif()

    #---------------------------------------------------------------------------
    # Populates CUDA_NVCC_FLAGS with the best set of flags to compile for a
    # given GPU architecture. The majority of developers should leave the
    # option at the default of 'native' which uses system introspection to
    # determine the smallest numerous of virtual and real architectures it
    # should target.
    #
    # The option of 'all' is provided for people generating libraries that
    # will deployed to any number of machines, it will compile all CUDA code
    # for all major virtual architectures, guaranteeing that the code will run
    # anywhere.
    #
    #
    # 1 - native
    #   - Uses system introspection to determine compile flags
    # 2 - fermi
    #   - Uses: --generate-code=arch=compute_20,code=compute_20
    # 3 - kepler
    #   - Uses: --generate-code=arch=compute_30,code=compute_30
    #   - Uses: --generate-code=arch=compute_35,code=compute_35
    # 4 - maxwell
    #   - Uses: --generate-code=arch=compute_50,code=compute_50
    #   - Uses: --generate-code=arch=compute_52,code=compute_52
    # 5 - pascal
    #   - Uses: --generate-code=arch=compute_60,code=compute_60
    #   - Uses: --generate-code=arch=compute_61,code=compute_61
    # 6 - volta
    #   - Uses: --generate-code=arch=compute_70,code=compute_70
    # 7 - all
    #   - Uses: --generate-code=arch=compute_20,code=compute_20
    #   - Uses: --generate-code=arch=compute_30,code=compute_30
    #   - Uses: --generate-code=arch=compute_35,code=compute_35
    #   - Uses: --generate-code=arch=compute_50,code=compute_50
    #   - Uses: --generate-code=arch=compute_52,code=compute_52
    #   - Uses: --generate-code=arch=compute_60,code=compute_60
    #   - Uses: --generate-code=arch=compute_61,code=compute_61
    #   - Uses: --generate-code=arch=compute_70,code=compute_70
    #

    #specify the property
    set(VTKm_CUDA_Architecture "native" CACHE STRING "Which GPU Architecture(s) to compile for")
    set_property(CACHE VTKm_CUDA_Architecture PROPERTY STRINGS native fermi kepler maxwell pascal volta all)

    #detect what the propery is set too
    if(VTKm_CUDA_Architecture STREQUAL "native")

      if(VTKM_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT)
        #Use the cached value
        list(APPEND CUDA_NVCC_FLAGS ${VTKM_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT})
      else()

        #run execute_process to do auto_detection
        if(CMAKE_GENERATOR MATCHES "Visual Studio")
          set(args "-ccbin" "${CMAKE_CXX_COMPILER}" "--run" "${VTKm_CMAKE_MODULE_PATH}/VTKmDetectCUDAVersion.cu")
        elseif(CUDA_HOST_COMPILER)
          set(args "-ccbin" "${CUDA_HOST_COMPILER}" "--run" "${VTKm_CMAKE_MODULE_PATH}/VTKmDetectCUDAVersion.cu")
        else()
          set(args "--run" "${VTKm_CMAKE_MODULE_PATH}/VTKmDetectCUDAVersion.cu")
        endif()

        execute_process(
          COMMAND ${CUDA_NVCC_EXECUTABLE} ${args}
          RESULT_VARIABLE ran_properly
          OUTPUT_VARIABLE run_output
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        if(ran_properly EQUAL 0)
          #find the position of the "--generate-code" output. With some compilers such as
          #msvc we get compile output plus run output. So we need to strip out just the
          #run output
          string(FIND "${run_output}" "--generate-code" position)
          string(SUBSTRING "${run_output}" ${position} -1 run_output)

          list(APPEND CUDA_NVCC_FLAGS ${run_output})
          set(VTKM_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT ${run_output} CACHE INTERNAL
              "device type(s) for cuda[native]")
        else()
          set(VTKm_CUDA_Architecture "fermi")
          vtkm_configure_component_message("Unable to run ${CUDA_NVCC_EXECUTABLE} to autodetect GPU architecture. Falling back to fermi, please manually specify if you want something else.")
        endif()
      endif()
    endif()

    #since when we are native we can fail, and fall back to "fermi" these have
    #to happen after, and separately of the native check
    if(VTKm_CUDA_Architecture STREQUAL "fermi")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_20,code=compute_20")
    elseif(VTKm_CUDA_Architecture STREQUAL "kepler")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_30,code=compute_30")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_35,code=compute_35")
    elseif(VTKm_CUDA_Architecture STREQUAL "maxwell")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_50,code=compute_50")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_52,code=compute_52")
    elseif(VTKm_CUDA_Architecture STREQUAL "pascal")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_60,code=compute_60")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_61,code=compute_61")
    elseif(VTKm_CUDA_Architecture STREQUAL "volta")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_70,code=compute_70")
    elseif(VTKm_CUDA_Architecture STREQUAL "all")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_20,code=compute_20")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_30,code=compute_30")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_35,code=compute_35")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_50,code=compute_50")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_52,code=compute_52")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_60,code=compute_60")
      list(APPEND CUDA_NVCC_FLAGS "--generate-code=arch=compute_61,code=compute_61")
    endif()

    if(WIN32)
      # On Windows, there is an issue with performing parallel builds with
      # nvcc. Multiple compiles can attempt to write the same .pdb file. Add
      # this argument to avoid this problem.
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --compiler-options /FS")
    endif()
  endif()
endmacro(vtkm_configure_component_CUDA)
