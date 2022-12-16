# VTK-m #

VTK-m is a toolkit of scientific visualization algorithms for emerging
processor architectures. VTK-m supports the fine-grained concurrency for
data analysis and visualization algorithms required to drive extreme scale
computing by providing abstract models for data and execution that can be
applied to a variety of algorithms across many different processor
architectures.

You can find out more about the design of VTK-m on the [VTK-m Wiki].


## Learning Resources ##

  + A high-level overview is given in the IEEE Vis talk "[VTK-m:
    Accelerating the Visualization Toolkit for Massively Threaded
    Architectures][VTK-m Overview]."

  + The [VTK-m Users Guide] provides extensive documentation. It is broken
    into multiple parts for learning and references at multiple different
    levels.
      + "Part 1: Getting Started" provides the introductory instruction for
        building VTK-m and using its high-level features.
      + "Part 2: Using VTK-m" covers the core fundamental components of
        VTK-m including data model, worklets, and filters.
      + "Part 3: Developing with VTK-m" covers how to develop new worklets
        and filters.
      + "Part 4: Advanced Development" covers topics such as new worklet
        types and custom device adapters.

  + A practical [VTK-m Tutorial] based in what users want to accomplish with
    VTK-m:
      + Building VTK-m and using existing VTK-m data structures and filters.
      + Algorithm development with VTK-m.
      + Writing new VTK-m filters.

  + Community discussion takes place on the [VTK-m users email list].

  + Doxygen-generated reference documentation is available for both:
    + Last Nightly build [VTK-m Doxygen nightly]
    + Last release [VTK-m Doxygen latest]


## Contributing ##

There are many ways to contribute to [VTK-m], with varying levels of
effort.

  + Ask a question on the [VTK-m users email list].

  + Submit new or add to discussions of a feature requests or bugs on the
    [VTK-m Issue Tracker].

  + Submit a Pull Request to improve [VTK-m]
      + See [CONTRIBUTING.md] for detailed instructions on how to create a
        Pull Request.
      + See the [VTK-m Coding Conventions] that must be followed for
        contributed code.

  + Submit an Issue or Pull Request for the [VTK-m Users Guide]


## Dependencies ##

VTK-m Requires:

  + C++11 Compiler. VTK-m has been confirmed to work with the following
      + GCC 5.4+
      + Clang 5.0+
      + XCode 5.0+
      + MSVC 2015+
      + Intel 17.0.4+
  + [CMake](http://www.cmake.org/download/)
      + CMake 3.12+
      + CMake 3.13+ (for CUDA support)

Optional dependencies are:

  + CUDA Device Adapter
      + [Cuda Toolkit 9.2, >= 10.2](https://developer.nvidia.com/cuda-toolkit)
      + Note CUDA >= 10.2 is required on Windows
  + TBB Device Adapter
      + [TBB](https://www.threadingbuildingblocks.org/)
  + OpenMP Device Adapter
      + Requires a compiler that supports OpenMP >= 4.0.
  + OpenGL Rendering
      + The rendering module contains multiple rendering implementations
        including standalone rendering code. The rendering module also
        includes (optionally built) OpenGL rendering classes.
      + The OpenGL rendering classes require that you have a extension
        binding library and one rendering library. A windowing library is
        not needed except for some optional tests.
  + Extension Binding
      + [GLEW](http://glew.sourceforge.net/)
  + On Screen Rendering
      + OpenGL Driver
      + Mesa Driver
  + On Screen Rendering Tests
      + [GLFW](http://www.glfw.org/)
      + [GLUT](http://freeglut.sourceforge.net/)
  + Headless Rendering
      + [OS Mesa](https://www.mesa3d.org/osmesa.html)
      + EGL Driver

VTK-m has been tested on the following configurations:c
  + On Linux
      + GCC 5.4.0, 5.4, 6.5, 7.4, 8.2, 9.2; Clang 5, 8; Intel 17.0.4; 19.0.0
      + CMake 3.12, 3.13, 3.16, 3.17
      + CUDA 9.2, 10.2, 11.0, 11.1 
      + TBB 4.4 U2, 2017 U7
  + On Windows
      + Visual Studio 2015, 2017
      + CMake 3.12, 3.17
      + CUDA 10.2
      + TBB 2017 U3, 2018 U2
  + On MacOS
      + AppleClang 9.1
      + CMake 3.12
      + TBB 2018


## Building ##

VTK-m supports all majors platforms (Windows, Linux, OSX), and uses CMake
to generate all the build rules for the project. The VTK-m source code is
available from the [VTK-m download page] or by directly cloning the [VTK-m
git repository].

The basic procedure for building VTK-m is to unpack the source, create a
build directory, run CMake in that build directory (pointing to the source)
and then build. Here are some example *nix commands for the process
(individual commands may vary).

```sh
$ tar xvzf ~/Downloads/vtk-m-v2.0.0.tar.gz
$ mkdir vtkm-build
$ cd vtkm-build
$ cmake-gui ../vtk-m-v2.0.0
$ cmake --build -j .              # Runs make (or other build program)
```

A more detailed description of building VTK-m is available in the [VTK-m
Users Guide].


## Example ##

The VTK-m source distribution includes a number of examples. The goal of the
VTK-m examples is to illustrate specific VTK-m concepts in a consistent and
simple format. However, these examples only cover a small portion of the
capabilities of VTK-m.

Below is a simple example of using VTK-m to create a simple data set and use VTK-m's rendering
engine to render an image and write that image to a file. It then computes an isosurface on the
input data set and renders this output data set in a separate image file:

```cpp
#include <vtkm/cont/Initialize.h>
#include <vtkm/source/Tangle.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

#include <vtkm/filter/contour/Contour.h>

using vtkm::rendering::CanvasRayTracer;
using vtkm::rendering::MapperRayTracer;
using vtkm::rendering::MapperVolume;
using vtkm::rendering::MapperWireframer;

int main(int argc, char* argv[])
{
  vtkm::cont::Initialize(argc, argv, vtkm::cont::InitializeOptions::Strict);

  auto tangle = vtkm::source::Tangle(vtkm::Id3{ 50, 50, 50 });
  vtkm::cont::DataSet tangleData = tangle.Execute();
  std::string fieldName = "tangle";

  // Set up a camera for rendering the input data
  vtkm::rendering::Camera camera;
  camera.SetLookAt(vtkm::Vec3f_32(0.5, 0.5, 0.5));
  camera.SetViewUp(vtkm::make_Vec(0.f, 1.f, 0.f));
  camera.SetClippingRange(1.f, 10.f);
  camera.SetFieldOfView(60.f);
  camera.SetPosition(vtkm::Vec3f_32(1.5, 1.5, 1.5));
  vtkm::cont::ColorTable colorTable("inferno");

  // Background color:
  vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  vtkm::rendering::Actor actor(tangleData.GetCellSet(),
                               tangleData.GetCoordinateSystem(),
                               tangleData.GetField(fieldName),
                               colorTable);
  vtkm::rendering::Scene scene;
  scene.AddActor(actor);
  // 2048x2048 pixels in the canvas:
  CanvasRayTracer canvas(2048, 2048);
  // Create a view and use it to render the input data using OS Mesa

  vtkm::rendering::View3D view(scene, MapperVolume(), canvas, camera, bg);
  view.Paint();
  view.SaveAs("volume.png");

  // Compute an isosurface:
  vtkm::filter::contour::Contour filter;
  // [min, max] of the tangle field is [-0.887, 24.46]:
  filter.SetIsoValue(3.0);
  filter.SetActiveField(fieldName);
  vtkm::cont::DataSet isoData = filter.Execute(tangleData);
  // Render a separate image with the output isosurface
  vtkm::rendering::Actor isoActor(
    isoData.GetCellSet(), isoData.GetCoordinateSystem(), isoData.GetField(fieldName), colorTable);
  // By default, the actor will automatically scale the scalar range of the color table to match
  // that of the data. However, we are coloring by the scalar that we just extracted a contour
  // from, so we want the scalar range to match that of the previous image.
  isoActor.SetScalarRange(actor.GetScalarRange());
  vtkm::rendering::Scene isoScene;
  isoScene.AddActor(isoActor);

  // Wireframe surface:
  vtkm::rendering::View3D isoView(isoScene, MapperWireframer(), canvas, camera, bg);
  isoView.Paint();
  isoView.SaveAs("isosurface_wireframer.png");

  // Smooth surface:
  vtkm::rendering::View3D solidView(isoScene, MapperRayTracer(), canvas, camera, bg);
  solidView.Paint();
  solidView.SaveAs("isosurface_raytracer.png");

  return 0;
}
```

A minimal CMakeLists.txt such as the following one can be used to build this
example.

```CMake
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(VTKmDemo CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(TARGET vtkm::rendering)
  add_executable(Demo Demo.cxx)
  target_link_libraries(Demo PRIVATE vtkm::filter vtkm::rendering vtkm::source)
endif()
```

## License ##

VTK-m is distributed under the OSI-approved BSD 3-clause License.
See [LICENSE.txt](LICENSE.txt) for details.


[VTK-m]:                    https://gitlab.kitware.com/vtk/vtk-m/
[VTK-m Coding Conventions]: docs/CodingConventions.md
[VTK-m Doxygen latest]:     https://docs-m.vtk.org/latest/index.html
[VTK-m Doxygen nightly]:    https://docs-m.vtk.org/nightly/
[VTK-m download page]:      https://gitlab.kitware.com/vtk/vtk-m/-/releases
[VTK-m git repository]:     https://gitlab.kitware.com/vtk/vtk-m/
[VTK-m Issue Tracker]:      https://gitlab.kitware.com/vtk/vtk-m/-/issues
[VTK-m Overview]:           http://m.vtk.org/images/2/29/VTKmVis2016.pptx
[VTK-m Users Guide]:        https://gitlab.kitware.com/vtk/vtk-m-user-guide/-/wikis/home
[VTK-m users email list]:   http://vtk.org/mailman/listinfo/vtkm
[VTK-m Wiki]:               http://m.vtk.org/
[VTK-m Tutorial]:           http://m.vtk.org/index.php/Tutorial
[CONTRIBUTING.md]:          CONTRIBUTING.md
