cmake_minimum_required(VERSION 3.8...3.14 FATAL_ERROR)
project(ParticleAdvection CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(ftle LagrangianStructures.cxx)
target_link_libraries(ftle PRIVATE vtkm_cont vtkm_worklet)
vtkm_add_target_information(ftle
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES  LagrangianStructures.cxx)
if(TARGET vtkm::tbb)
  target_compile_definitions(ftle PRIVATE BUILDING_TBB_VERSION)
endif()
