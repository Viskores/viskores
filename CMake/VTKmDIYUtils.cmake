##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

macro(_vtkm_diy_target flag target)
  set(${target} "vtkmdiympi")
  if (NOT ${flag})
    set(${target} "vtkmdiympi_nompi")
  endif()
endmacro()

function(vtkm_diy_init_target)
  set(vtkm_diy_default_flag "${VTKm_ENABLE_MPI}")
  _vtkm_diy_target(vtkm_diy_default_flag vtkm_diy_default_target)

  set_target_properties(vtkm_diy PROPERTIES
    vtkm_diy_use_mpi_stack ${vtkm_diy_default_flag}
    vtkm_diy_target ${vtkm_diy_default_target})
endfunction()

#-----------------------------------------------------------------------------
function(vtkm_diy_use_mpi_push)
  set(topval ${VTKm_ENABLE_MPI})
  if (NOT ARGC EQUAL 0)
    set(topval ${ARGV0})
  endif()
  get_target_property(stack vtkm_diy vtkm_diy_use_mpi_stack)
  list (APPEND stack ${topval})
  _vtkm_diy_target(topval target)
  set_target_properties(vtkm_diy PROPERTIES
    vtkm_diy_use_mpi_stack "${stack}"
    vtkm_diy_target "${target}")
endfunction()

function(vtkm_diy_use_mpi value)
  get_target_property(stack vtkm_diy vtkm_diy_use_mpi_stack)
  list (REMOVE_AT stack -1)
  list (APPEND stack ${value})
  _vtkm_diy_target(value target)
  set_target_properties(vtkm_diy PROPERTIES
    vtkm_diy_use_mpi_stack "${stack}"
    vtkm_diy_target "${target}")
endfunction()

function(vtkm_diy_use_mpi_pop)
  get_target_property(stack vtkm_diy vtkm_diy_use_mpi_stack)
  list (GET stack -1 value)
  list (REMOVE_AT stack -1)
  _vtkm_diy_target(value target)
  set_target_properties(vtkm_diy PROPERTIES
    vtkm_diy_use_mpi_stack "${stack}"
    vtkm_diy_target "${target}")
endfunction()
