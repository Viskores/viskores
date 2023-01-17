##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

macro(vtkm_diy_get_general_target target)
  if(PROJECT_NAME STREQUAL "VTKm" OR CMAKE_PROJECT_NAME STREQUAL "VTKm")
    set(${target} "vtkm_diy")
  else()
    set(${target} "vtkm::diy")
  endif()
endmacro()

macro(_vtkm_diy_target flag target)
  set(${target} "vtkmdiympi")
  if (NOT ${flag})
    set(${target} "vtkmdiympi_nompi")
  endif()
endmacro()

function(vtkm_diy_init_target)
  set(vtkm_diy_default_flag "${VTKm_ENABLE_MPI}")
  _vtkm_diy_target(vtkm_diy_default_flag vtkm_diy_default_target)

  vtkm_diy_get_general_target(diy_target)
  set_target_properties(${diy_target} PROPERTIES
    vtkm_diy_use_mpi_stack ${vtkm_diy_default_flag}
    vtkm_diy_target ${vtkm_diy_default_target})
endfunction()

#-----------------------------------------------------------------------------
function(vtkm_diy_use_mpi_push)
  set(topval ${VTKm_ENABLE_MPI})
  if (NOT ARGC EQUAL 0)
    set(topval ${ARGV0})
  endif()
  vtkm_diy_get_general_target(diy_target)
  get_target_property(stack ${diy_target} vtkm_diy_use_mpi_stack)
  list (APPEND stack ${topval})
  _vtkm_diy_target(topval target)
  set_target_properties(${diy_target} PROPERTIES
    vtkm_diy_use_mpi_stack "${stack}"
    vtkm_diy_target "${target}")
endfunction()

function(vtkm_diy_use_mpi value)
  vtkm_diy_get_general_target(diy_target)
  get_target_property(stack ${diy_target} vtkm_diy_use_mpi_stack)
  list (REMOVE_AT stack -1)
  list (APPEND stack ${value})
  _vtkm_diy_target(value target)
  set_target_properties(${diy_target} PROPERTIES
    vtkm_diy_use_mpi_stack "${stack}"
    vtkm_diy_target "${target}")
endfunction()

function(vtkm_diy_use_mpi_pop)
  vtkm_diy_get_general_target(diy_target)
  get_target_property(stack ${diy_target} vtkm_diy_use_mpi_stack)
  list (GET stack -1 value)
  list (REMOVE_AT stack -1)
  _vtkm_diy_target(value target)
  set_target_properties(${diy_target} PROPERTIES
    vtkm_diy_use_mpi_stack "${stack}"
    vtkm_diy_target "${target}")
endfunction()
