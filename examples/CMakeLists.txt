##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

if(VTKm_ENABLE_EXAMPLES)
  # VTKm examples expects vtkm libraries to be namespaced with the prefix vtkm::.
  # However as the examples are also built as just another part of the VTK-m code
  # those prefix are not added to the targets (This happens during the
  # installation). To workaround this issue here, we create IMPORTED libs linking
  # to the vtkm libraries used by the examples with expected the vtkm:: prefix.
  vtkm_module_get_list(module_list)
  foreach(tgt IN LISTS module_list)
    if(TARGET ${tgt})
      # The reason of creating this phony IMPORTED libraries instead of making
      # ALIAS libraries is that ALIAS libraries are GLOBAL whereas IMPORTED are
      # local at the directory level where they are created. We do not want these
      # phony targets to be visible outside of the example directory.
      vtkm_target_mangle(tgt_name_mangled ${tgt})
      add_library("vtkm::${tgt_name_mangled}" INTERFACE IMPORTED)
      target_link_libraries("vtkm::${tgt_name_mangled}" INTERFACE ${tgt})
    endif()
  endforeach()

  #add the directory that contains the VTK-m config file to the cmake
  #path so that our examples can find VTK-m
  set(CMAKE_PREFIX_PATH ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR})
  add_subdirectory(clipping)
  add_subdirectory(contour_tree)
  add_subdirectory(contour_tree_augmented)
  add_subdirectory(contour_tree_distributed)
  add_subdirectory(cosmotools)
  add_subdirectory(demo)
  add_subdirectory(game_of_life)
  add_subdirectory(hello_worklet)
  add_subdirectory(histogram)
  add_subdirectory(ising)
  add_subdirectory(lagrangian)
  add_subdirectory(logistic_map)
  add_subdirectory(mesh_quality)
  add_subdirectory(multi_backend)
  add_subdirectory(oscillator)
  add_subdirectory(particle_advection)
  add_subdirectory(streamline_mpi)
  add_subdirectory(polyline_archimedean_helix)
  add_subdirectory(redistribute_points)
  add_subdirectory(temporal_advection)
  add_subdirectory(tetrahedra)
  add_subdirectory(smoke_test)
endif()

if (VTKm_ENABLE_TESTING)
  # These need to be fast to build as they will
  # be built each time we run the test
  vtkm_test_against_install(demo)
  vtkm_test_against_install(histogram)
  vtkm_test_against_install(smoke_test)
endif()
