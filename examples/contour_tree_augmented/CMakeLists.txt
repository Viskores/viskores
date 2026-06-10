##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

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
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# Find the Viskores package
find_package(Viskores REQUIRED QUIET)

if(NOT TARGET viskores::io OR NOT TARGET viskores::filter_scalar_topology)
  # Libraries needed are not built.
  return()
endif()

####################################
# Serial
####################################
add_executable(ContourTree_Augmented ContourTreeApp.cxx)
target_link_libraries(ContourTree_Augmented PUBLIC viskores::filter_scalar_topology viskores::io viskores::worklet)
viskores_add_target_information(ContourTree_Augmented
                            DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES ContourTreeApp.cxx)

option (VISKORES_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT Off)
mark_as_advanced(VISKORES_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT)
if (VISKORES_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT)
  target_compile_definitions(ContourTree_Augmented PRIVATE "DEBUG_PRINT")
endif()

if (TARGET viskores::tbb)
  target_compile_definitions(ContourTree_Augmented PRIVATE "ENABLE_SET_NUM_THREADS")
endif()

####################################
# MPI
####################################
if (Viskores_ENABLE_MPI)
  add_executable(ContourTree_Augmented_MPI ContourTreeApp.cxx)
  target_link_libraries(ContourTree_Augmented_MPI viskores::filter_scalar_topology viskores::io viskores::worklet MPI::MPI_CXX)
  viskores_add_target_information(ContourTree_Augmented_MPI
                              MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES ContourTreeApp.cxx)
  target_compile_definitions(ContourTree_Augmented_MPI PRIVATE "WITH_MPI")

  option (VISKORES_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT Off)
  mark_as_advanced(VISKORES_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT)
  if (VISKORES_EXAMPLE_CONTOURTREE_ENABLE_DEBUG_PRINT)
    target_compile_definitions(ContourTree_Augmented_MPI PRIVATE "DEBUG_PRINT")
  endif()

  if (TARGET viskores::tbb)
    target_compile_definitions(ContourTree_Augmented_MPI PRIVATE "ENABLE_SET_NUM_THREADS")
  endif()
endif()
