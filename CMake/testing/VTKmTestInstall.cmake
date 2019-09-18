##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# -----------------------------------------------------------------------------
function(vtkm_test_install )
  if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
    set(command_args
      "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}"
      "-DVTKm_BINARY_DIR=${VTKm_BINARY_DIR}"
      "-DVTKm_INSTALL_INCLUDE_DIR=${VTKm_INSTALL_INCLUDE_DIR}"
      "-DVTKm_ENABLE_RENDERING=${VTKm_ENABLE_RENDERING}"
      "-DVTKm_ENABLE_LOGGING=${VTKm_ENABLE_LOGGING}"
      )

    #By having this as separate tests using fixtures, it will allow us in
    #the future to write tests that build against the installed version
    add_test(NAME TestInstallSetup
      COMMAND ${CMAKE_COMMAND}
        "-DMODE=INSTALL"
        ${command_args}
        -P "${VTKm_SOURCE_DIR}/CMake/testing/VTKmCheckSourceInInstall.cmake"
      )

    add_test(NAME SourceInInstall
      COMMAND ${CMAKE_COMMAND}
        "-DMODE=VERIFY"
        ${command_args}
        -P "${VTKm_SOURCE_DIR}/CMake/testing/VTKmCheckSourceInInstall.cmake"
      )

    add_test(NAME TestInstallCleanup
      COMMAND ${CMAKE_COMMAND}
       "-DMODE=CLEANUP"
        ${command_args}
        -P "${VTKm_SOURCE_DIR}/CMake/testing/VTKmCheckSourceInInstall.cmake"
      )

    set_tests_properties(TestInstallSetup PROPERTIES FIXTURES_SETUP vtkm_installed)
    set_tests_properties(SourceInInstall PROPERTIES FIXTURES_REQUIRED vtkm_installed)
    set_tests_properties(TestInstallCleanup PROPERTIES FIXTURES_CLEANUP vtkm_installed)

    set_tests_properties(SourceInInstall PROPERTIES LABELS "TEST_INSTALL" )
  endif()
endfunction()

# -----------------------------------------------------------------------------
function(vtkm_generate_install_build_options file_loc_var)
#This generated file ensures that the adaptor's CMakeCache ends up with
#the same CMAKE_PREFIX_PATH that VTK-m's does, even if that has multiple
#paths in it. It is necessary because ctest's argument parsing in the
#custom command below destroys path separators.
#Note: the generated file will become stale if these variables change.
#In that case it will need manual intervention (remove it) to fix.
file(GENERATE
  OUTPUT "${${file_loc_var}}"
  CONTENT
"
set(CMAKE_MAKE_PROGRAM \"${CMAKE_MAKE_PROGRAM}\" CACHE FILEPATH \"\")
set(CMAKE_PREFIX_PATH \"${CMAKE_PREFIX_PATH};${install_prefix}/\" CACHE STRING \"\")
set(CMAKE_CXX_COMPILER \"${CMAKE_CXX_COMPILER}\" CACHE FILEPATH \"\")
set(CMAKE_CXX_FLAGS \"$CACHE{CMAKE_CXX_FLAGS}\" CACHE STRING \"\")
set(CMAKE_CUDA_COMPILER \"${CMAKE_CUDA_COMPILER}\" CACHE FILEPATH \"\")
set(CMAKE_CUDA_FLAGS \"$CACHE{CMAKE_CUDA_FLAGS}\" CACHE STRING \"\")
set(CMAKE_CUDA_HOST_COMPILER \"${CMAKE_CUDA_HOST_COMPILER}\" CACHE FILEPATH \"\")
"
)

endfunction()

# -----------------------------------------------------------------------------
function(vtkm_test_against_install dir)
  set(name ${dir})
  set(install_prefix "${VTKm_BINARY_DIR}/CMakeFiles/_tmp_install")
  set(src_dir "${CMAKE_CURRENT_SOURCE_DIR}/${name}/")
  set(build_dir "${VTKm_BINARY_DIR}/CMakeFiles/_tmp_build/test_${name}/")

  set(args )
  if(CMAKE_VERSION VERSION_LESS 3.13)
    #Before 3.13 the config file passing to cmake via ctest --build-options
    #was broken
    set(args
      -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}
      -DCMAKE_PREFIX_PATH:STRING=${install_prefix}
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CUDA_COMPILER:FILEPATH=${CMAKE_CUDA_COMPILER}
      -DCMAKE_CUDA_HOST_COMPILER:FILEPATH=${CMAKE_CUDA_HOST_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=$CACHE{CMAKE_CXX_FLAGS}
      -DCMAKE_CUDA_FLAGS:STRING=$CACHE{CMAKE_CUDA_FLAGS}
    )
  else()
    set(build_config "${build_dir}build_options.cmake")
    vtkm_generate_install_build_options(build_config)
    set(args -C ${build_config})
  endif()

  if(WIN32 AND TARGET vtkm::tbb)
    #on windows we need to specify these as FindTBB won't
    #find the installed version just with the prefix path
    list(APPEND args
      -DTBB_LIBRARY_DEBUG:FILEPATH=${TBB_LIBRARY_DEBUG}
      -DTBB_LIBRARY_RELEASE:FILEPATH=${TBB_LIBRARY_RELEASE}
      -DTBB_INCLUDE_DIR:PATH=${TBB_INCLUDE_DIR}
    )
  endif()

  #determine if the test is expected to compile or fail to build. We use
  #this information to built the test name to make it clear to the user
  #what a 'passing' test means
  set(retcode 0)
  set(build_name "${name}_built_against_test_install")
  set(test_label "TEST_INSTALL")

  add_test(NAME ${build_name}
           COMMAND ${CMAKE_CTEST_COMMAND}
           -C $<CONFIG>
           --build-and-test ${src_dir} ${build_dir}
           --build-generator ${CMAKE_GENERATOR}
           --build-makeprogram ${CMAKE_MAKE_PROGRAM}
           --build-options
            ${args}
            --no-warn-unused-cli
           )

  set_tests_properties(${build_name} PROPERTIES LABELS ${test_label} )
  set_tests_properties(${build_name} PROPERTIES FIXTURES_REQUIRED vtkm_installed)
  set_tests_properties(${build_name} PROPERTIES TIMEOUT 600)
endfunction()
