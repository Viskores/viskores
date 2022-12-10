##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

## Copyright (c) 2018, The Regents of the University of California, through
## Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
## from the U.S. Dept. of Energy).  All rights reserved.
##
## Redistribution and use in source and binary forms, with or without modification,
## are permitted provided that the following conditions are met:
##
## (1) Redistributions of source code must retain the above copyright notice, this
##     list of conditions and the following disclaimer.
##
## (2) Redistributions in binary form must reproduce the above copyright notice,
##     this list of conditions and the following disclaimer in the documentation
##     and/or other materials provided with the distribution.
##
## (3) Neither the name of the University of California, Lawrence Berkeley National
##     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
##     used to endorse or promote products derived from this software without
##     specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
## ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
## BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
## LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
## OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
## OF THE POSSIBILITY OF SUCH DAMAGE.
##
##=============================================================================
##
##  This code is an extension of the algorithm presented in the paper:
##  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
##  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
##  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
##  (LDAV), October 2016, Baltimore, Maryland.
##
##  The PPP2 algorithm and software were jointly developed by
##  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
##  Oliver Ruebel (LBNL)
##==============================================================================
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)

# Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

####################################
# MPI
####################################
if (VTKm_ENABLE_MPI AND TARGET vtkm::filter_scalar_topology AND TARGET vtkm::io)
  add_executable(ContourTree_Distributed ContourTreeApp.cxx)
  target_link_libraries(ContourTree_Distributed vtkm::filter_scalar_topology vtkm::io MPI::MPI_CXX)
  vtkm_add_target_information(ContourTree_Distributed
                              MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES ContourTreeApp.cxx)
  target_compile_definitions(ContourTree_Distributed PRIVATE "WITH_MPI")

  option (VTKM_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT Off)
  mark_as_advanced(VTKM_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT)
  if (VTKM_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT)
    target_compile_definitions(ContourTree_Distributed PRIVATE "DEBUG_PRINT")
    target_compile_definitions(ContourTree_Distributed PRIVATE "DEBUG_PRINT_CTUD")
  endif()

  if (TARGET vtkm::tbb)
    target_compile_definitions(ContourTree_Distributed PRIVATE "ENABLE_SET_NUM_THREADS")
  endif()

  add_executable(TreeCompiler TreeCompilerApp.cxx)
  target_link_libraries(TreeCompiler vtkm::filter_core)
  vtkm_add_target_information(TreeCompiler DROP_UNUSED_SYMBOLS)

  add_executable(BranchCompiler BranchCompilerApp.cxx)
  target_link_libraries(BranchCompiler vtkm::filter)
  vtkm_add_target_information(BranchCompiler DROP_UNUSED_SYMBOLS)

  configure_file(split_data_2d.py split_data_2d.py COPYONLY)
  configure_file(split_data_3d.py split_data_3d.py COPYONLY)
  configure_file(hact_test.sh hact_test.sh COPYONLY)
  configure_file(hact_test_volume.sh hact_test_volume.sh COPYONLY)
  configure_file(hact_test_branch_decomposition.sh hact_test_branch_decomposition.sh COPYONLY)
  configure_file(testrun_branch_decomposition.sh testrun_branch_decomposition.sh COPYONLY)
  configure_file(testrun.sh testrun.sh COPYONLY)
  configure_file(testrun_volume.sh testrun_volume.sh COPYONLY)
endif()
