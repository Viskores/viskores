find_package(VTKm REQUIRED QUIET
  OPTIONAL_COMPONENTS Serial #CUDA TBB OpenGL GLUT
  )

add_executable(CCL CCL.cpp)
target_link_libraries(CCL PRIVATE ${VTKm_LIBRARIES})
target_compile_options(CCL PRIVATE ${VTKm_COMPILE_OPTIONS})
