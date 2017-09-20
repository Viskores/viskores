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

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET
  OPTIONAL_COMPONENTS Serial CUDA TBB
  )

add_executable(ContourTreeMesh2D_SERIAL ContourTreeMesh2D.cxx)
target_include_directories(ContourTreeMesh2D_SERIAL PRIVATE ${VTKm_INCLUDE_DIRS})
target_link_libraries(ContourTreeMesh2D_SERIAL      PRIVATE ${VTKm_LIBRARIES})
target_compile_options(ContourTreeMesh2D_SERIAL     PRIVATE ${VTKm_COMPILE_OPTIONS})

add_executable(ContourTreeMesh3D_SERIAL ContourTreeMesh3D.cxx)
target_include_directories(ContourTreeMesh3D_SERIAL PRIVATE ${VTKm_INCLUDE_DIRS})
target_link_libraries(ContourTreeMesh3D_SERIAL      PRIVATE ${VTKm_LIBRARIES})
target_compile_options(ContourTreeMesh3D_SERIAL     PRIVATE ${VTKm_COMPILE_OPTIONS})

if(VTKm_CUDA_FOUND)
  set(old_nvcc_flags ${CUDA_NVCC_FLAGS})
  set(old_cxx_flags ${CMAKE_CXX_FLAGS})
  vtkm_setup_nvcc_flags( old_nvcc_flags old_cxx_flags)
  vtkm_disable_troublesome_thrust_warnings()

  # Cuda compiles do not respect target_include_directories
  cuda_include_directories(${VTKm_INCLUDE_DIRS})

  set (cudaSource "${CMAKE_CURRENT_BINARY_DIR}/ContourTreeMesh2D.cu")
  configure_file(ContourTreeMesh2D.cxx ${cudaSource} COPYONLY)
  cuda_add_executable(ContourTreeMesh2D_CUDA ${cudaSource})
  target_include_directories(ContourTreeMesh2D_CUDA PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(ContourTreeMesh2D_CUDA      PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(ContourTreeMesh2D_CUDA     PRIVATE ${VTKm_COMPILE_OPTIONS})

  set (cudaSource "${CMAKE_CURRENT_BINARY_DIR}/ContourTreeMesh3D.cu")
  configure_file(ContourTreeMesh3D.cxx ${cudaSource} COPYONLY)
  cuda_add_executable(ContourTreeMesh3D_CUDA ${cudaSource})
  target_include_directories(ContourTreeMesh3D_CUDA PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(ContourTreeMesh3D_CUDA      PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(ContourTreeMesh3D_CUDA     PRIVATE ${VTKm_COMPILE_OPTIONS})

  set(CUDA_NVCC_FLAGS ${old_nvcc_flags})
  set(CMAKE_CXX_FLAGS ${old_cxx_flags})
endif()

if(VTKm_TBB_FOUND)
  add_executable(ContourTreeMesh2D_TBB ContourTreeMesh2D.cxx)
  target_include_directories(ContourTreeMesh2D_TBB  PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(ContourTreeMesh2D_TBB       PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(ContourTreeMesh2D_TBB      PRIVATE ${VTKm_COMPILE_OPTIONS})

  add_executable(ContourTreeMesh3D_TBB ContourTreeMesh3D.cxx)
  target_include_directories(ContourTreeMesh3D_TBB  PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(ContourTreeMesh3D_TBB       PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(ContourTreeMesh3D_TBB      PRIVATE ${VTKm_COMPILE_OPTIONS})
endif()
