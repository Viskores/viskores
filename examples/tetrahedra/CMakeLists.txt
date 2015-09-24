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
##  Copyright 2015 Sandia Corporation.
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================

cmake_minimum_required(VERSION 2.8.11)
find_package(OpenGL)
find_package(GLUT)
find_package(TBB)

if(OPENGL_FOUND AND GLUT_FOUND)
  add_executable(TetrahedralizeExplicitGrid_SERIAL TetrahedralizeExplicitGrid.cxx)
  target_include_directories(TetrahedralizeExplicitGrid_SERIAL PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
  target_link_libraries(TetrahedralizeExplicitGrid_SERIAL ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

  add_executable(TriangulateExplicitGrid_SERIAL TriangulateExplicitGrid.cxx)
  target_include_directories(TriangulateExplicitGrid_SERIAL PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
  target_link_libraries(TriangulateExplicitGrid_SERIAL ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

  if(VTKm_Cuda_FOUND)
    cuda_add_executable(TetrahedralizeExplicitGrid_CUDA TetrahedralizeExplicitGrid.cu)
    target_link_libraries(TetrahedralizeExplicitGrid_CUDA ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

    cuda_add_executable(TriangulateExplicitGrid_CUDA TriangulateExplicitGrid.cu)
    target_link_libraries(TriangulateExplicitGrid_CUDA ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
  endif()

  if(VTKm_ENABLE_TBB)
    add_executable(TetrahedralizeExplicitGrid_TBB TetrahedralizeExplicitGridTBB.cxx)
    target_include_directories(TetrahedralizeExplicitGrid_TBB PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${TBB_INCLUDE_DIRS})
    target_link_libraries(TetrahedralizeExplicitGrid_TBB ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${TBB_LIBRARIES})

    add_executable(TriangulateExplicitGrid_TBB TriangulateExplicitGridTBB.cxx)
    target_include_directories(TriangulateExplicitGrid_TBB PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${TBB_INCLUDE_DIRS})
    target_link_libraries(TriangulateExplicitGrid_TBB ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${TBB_LIBRARIES})
  endif()

endif()
