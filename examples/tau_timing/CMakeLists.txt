##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
project(TauTiming CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(TARGET vtkm_rendering)

  add_executable(Tau_timing_SERIAL TauTiming.cxx)
  target_link_libraries(Tau_timing_SERIAL vtkm_cont)

  if(TARGET vtkm::tbb)
    add_executable(Tau_timing_TBB TauTimingTBB.cxx)
    target_link_libraries(Tau_timing_TBB vtkm_cont)
  endif()

  if(TARGET vtkm::cuda)
    add_executable(Tau_timing_CUDA TauTiming.cu)
    target_link_libraries(Tau_timing_CUDA PRIVATE vtkm_cont)
  endif()
endif()
