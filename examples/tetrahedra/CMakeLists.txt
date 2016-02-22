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

if(OPENGL_FOUND AND GLUT_FOUND)
  add_executable(TetrahedralizeExplicitGrid_SERIAL TetrahedralizeExplicitGrid.cxx)
  target_include_directories(TetrahedralizeExplicitGrid_SERIAL PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
  target_link_libraries(TetrahedralizeExplicitGrid_SERIAL ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
  target_compile_options(TetrahedralizeExplicitGrid_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

  add_executable(TriangulateExplicitGrid_SERIAL TriangulateExplicitGrid.cxx)
  target_include_directories(TriangulateExplicitGrid_SERIAL PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
  target_link_libraries(TriangulateExplicitGrid_SERIAL ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
  target_compile_options(TriangulateExplicitGrid_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

  add_executable(TetrahedralizeUniformGrid_SERIAL TetrahedralizeUniformGrid.cxx)
  target_include_directories(TetrahedralizeUniformGrid_SERIAL PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
  target_link_libraries(TetrahedralizeUniformGrid_SERIAL ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
  target_compile_options(TetrahedralizeUniformGrid_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

  add_executable(TriangulateUniformGrid_SERIAL TriangulateUniformGrid.cxx)
  target_include_directories(TriangulateUniformGrid_SERIAL PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
  target_link_libraries(TriangulateUniformGrid_SERIAL ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
  target_compile_options(TriangulateUniformGrid_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

  if(VTKm_CUDA_FOUND)
    cuda_add_executable(TetrahedralizeExplicitGrid_CUDA TetrahedralizeExplicitGrid.cu)
    target_link_libraries(TetrahedralizeExplicitGrid_CUDA ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
    target_compile_options(TetrahedralizeExplicitGrid_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})

    cuda_add_executable(TriangulateExplicitGrid_CUDA TriangulateExplicitGrid.cu)
    target_link_libraries(TriangulateExplicitGrid_CUDA ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
    target_compile_options(TriangulateExplicitGrid_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})

    cuda_add_executable(TetrahedralizeUniformGrid_CUDA TetrahedralizeUniformGrid.cu)
    target_link_libraries(TetrahedralizeUniformGrid_CUDA ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
    target_compile_options(TetrahedralizeUniformGrid_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})

    cuda_add_executable(TriangulateUniformGrid_CUDA TriangulateUniformGrid.cu)
    target_link_libraries(TriangulateUniformGrid_CUDA ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
    target_compile_options(TriangulateUniformGrid_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})
  endif()

  if(VTKm_ENABLE_TBB)
    add_executable(TetrahedralizeExplicitGrid_TBB TetrahedralizeExplicitGridTBB.cxx)
    target_include_directories(TetrahedralizeExplicitGrid_TBB PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    target_link_libraries(TetrahedralizeExplicitGrid_TBB ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
    target_compile_options(TetrahedralizeExplicitGrid_TBB PRIVATE ${VTKm_COMPILE_OPTIONS})

    add_executable(TriangulateExplicitGrid_TBB TriangulateExplicitGridTBB.cxx)
    target_include_directories(TriangulateExplicitGrid_TBB PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    target_link_libraries(TriangulateExplicitGrid_TBB ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
    target_compile_options(TriangulateExplicitGrid_TBB PRIVATE ${VTKm_COMPILE_OPTIONS})

    add_executable(TetrahedralizeUniformGrid_TBB TetrahedralizeUniformGridTBB.cxx)
    target_include_directories(TetrahedralizeUniformGrid_TBB PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    target_link_libraries(TetrahedralizeUniformGrid_TBB ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
    target_compile_options(TetrahedralizeUniformGrid_TBB PRIVATE ${VTKm_COMPILE_OPTIONS})

    add_executable(TriangulateUniformGrid_TBB TriangulateUniformGridTBB.cxx)
    target_include_directories(TriangulateUniformGrid_TBB PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    target_link_libraries(TriangulateUniformGrid_TBB ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
    target_compile_options(TriangulateUniformGrid_TBB PRIVATE ${VTKm_COMPILE_OPTIONS})
  endif()

endif()
