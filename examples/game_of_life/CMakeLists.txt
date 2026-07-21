##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(GameOfLife CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)
viskores_find_gl(OPTIONAL GL GLUT GLEW)

if(TARGET OpenGL::GL AND
   TARGET GLUT::GLUT AND
   TARGET GLEW::GLEW)

  add_executable(GameOfLife GameOfLife.cxx LoadShaders.h)
  target_link_libraries(GameOfLife PRIVATE viskores::filter_core OpenGL::GL GLEW::GLEW GLUT::GLUT)
  viskores_add_target_information(GameOfLife
                              DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES GameOfLife.cxx)
endif()
