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
function(test_against_installed_vtkm dir)
  set(name ${dir})
  set(install_prefix "${VTKm_BINARY_DIR}/CMakeFiles/_tmp_install")
  set(src_dir "${CMAKE_CURRENT_SOURCE_DIR}/${name}/")
  set(build_dir "${VTKm_BINARY_DIR}/CMakeFiles/_tmp_build/test_${name}/")


  #determine if the test is expected to compile or fail to build. We use
  #this information to built the test name to make it clear to the user
  #what a 'passing' test means
  set(retcode 0)
  set(build_name "${name}_built_against_test_install")
  set(test_label "TEST_INSTALL")

  add_test(NAME ${build_name}
           COMMAND ${CMAKE_CTEST_COMMAND}
           --build-and-test ${src_dir} ${build_dir}
           --build-generator ${CMAKE_GENERATOR}
           --build-makeprogram ${CMAKE_MAKE_PROGRAM}
           --build-options
              -DCMAKE_PREFIX_PATH:STRING=${install_prefix}
              -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
              -DCMAKE_CUDA_COMPILER:FILEPATH=${CMAKE_CUDA_COMPILER}
              -DCMAKE_CUDA_HOST_COMPILER:FILEPATH=${CMAKE_CUDA_HOST_COMPILER}
           )

  set_tests_properties(${build_name} PROPERTIES LABELS ${test_label} )
  set_tests_properties(${build_name} PROPERTIES FIXTURES_REQUIRED vtkm_installed)
endfunction()
