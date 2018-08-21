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
##  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2018 UT-Battelle, LLC.
##  Copyright 2018 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
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

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

# MAKE DEBUG BUILD
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -g -fsanitize=address")

# MAKE OPTIMIZED BUILD
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O3")

####################################
# Serial builds
####################################
# Serial 2D / 3D / MC
add_executable(ContourTree_PPP2_SERIAL ContourTreeApp.cxx)
target_link_libraries(ContourTree_PPP2_SERIAL vtkm_cont)
target_compile_definitions(ContourTree_PPP2_SERIAL PRIVATE
                           "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL")


####################################
# Serial debug builds
####################################

# Debug Serial 2D / 3D / MC
add_executable(ContourTree_PPP2_SERIAL_DEBUG ContourTreeApp.cxx)
target_link_libraries(ContourTree_PPP2_SERIAL_DEBUG vtkm_cont)
target_compile_definitions(ContourTree_PPP2_SERIAL_DEBUG PRIVATE
                           "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL" "DEBUG_PRINT")


if(TARGET vtkm::tbb)
  # TBB 2D/3D/MC
  add_executable(ContourTree_PPP2_TBB ContourTreeApp.cxx)
  target_link_libraries(ContourTree_PPP2_TBB vtkm_cont)
  target_compile_definitions(ContourTree_PPP2_TBB PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB" "ENABLE_SET_NUM_THREADS")

  # TBB 2D/3D/MC DEBUG
  add_executable(ContourTree_PPP2_TBB_DEBUG ContourTreeApp.cxx)
  target_link_libraries(ContourTree_PPP2_TBB_DEBUG vtkm_cont)
  target_compile_definitions(ContourTree_PPP2_TBB_DEBUG PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB" "ENABLE_SET_NUM_THREADS" "DEBUG_PRINT")

endif()

if(TARGET vtkm::cuda)
  # CUDA 2D/3D/MC
  vtkm_compile_as_cuda(cudaSource ContourTreeApp.cxx)
  add_executable(ContourTree_PPP2_CUDA ${cudaSource})
  target_link_libraries(ContourTree_PPP2_CUDA vtkm_cont)
  target_compile_definitions(ContourTree_PPP2_CUDA PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")

  # CUDA 2D/3D/MC
  # vtkm_compile_as_cuda(cudaSource ContourTreeApp.cxx)
  add_executable(ContourTree_PPP2_CUDA_DEBUG ${cudaSource})
  target_link_libraries(ContourTree_PPP2_CUDA_DEBUG vtkm_cont)
  target_compile_definitions(ContourTree_PPP2_CUDA_DEBUG PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA" "DEBUG_PRINT")
endif()
