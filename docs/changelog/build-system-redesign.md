# Build System Redesign and new minimum CMake

VTK-m CMake buildsystem was redesigned to be more declarative for consumers.
This was done by moving away from the previous component design and instead
to explicit targets. Additionally VTK-m now uses the native CUDA support
introduced in CMake 3.8 and has the following minimum CMake versions:
 - Visual Studio Generator requires CMake 3.11+
 - CUDA support requires CMake 3.9+
 - Otherwise CMake 3.3+ is supported

When VTK-m is found find_package it defines the following targets:
  - `vtkm_cont`
    - contains all common core functionality
    - always exists

  - `vtkm_rendering`  
    - contains all the rendering code
    - exists only when rendering is enabled
    - rendering also provides a `vtkm_find_gl` function
      - allows you to find the GL (EGL,MESA,Hardware), GLUT, and GLEW
        versions that VTK-m was built with.

VTK-m also provides targets that represent what device adapters it
was built to support. The pattern for these targets are `vtkm::<device>`.
Currently we don't provide a target for the serial device.

  - `vtkm::tbb` 
    - Target that contains tbb related link information
       implicitly linked to by `vtkm_cont` if tbb was enabled

  - `vtkm::cuda` 
    - Target that contains cuda related link information
       implicitly linked to by `vtkm_cont` if cuda was enabled       

VTK-m can be built with specific CPU architecture vectorization/optimization flags.
Consumers of the project can find these flags by looking at the `vtkm_vectorization_flags`
target.

So a project that wants to build an executable that uses vtk-m would look like:

```cmake

cmake_minimum_required(VERSION 3.3 FATAL_ERROR)
project(HellowWorld CXX)

#Find the VTK-m package. 
#Will automatically enable the CUDA language if needed ( and bump CMake minimum )

find_package(VTKm REQUIRED)

add_executable(HelloWorld HelloWorld.cxx)
target_link_libraries(HelloWorld PRIVATE vtkm_cont)

if(TARGET vtkm::cuda)
  set_source_files_properties(HelloWorld.cxx PROPERTIES LANGUAGE CUDA)
endif()

```



