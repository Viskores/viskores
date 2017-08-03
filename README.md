## VTK-m ##

VTK-m is a toolkit of scientific visualization algorithms for emerging processor
architectures. VTK-m supports the fine-grained concurrency for data analysis and
visualization algorithms required to drive extreme scale computing by providing
abstract models for data and execution that can be applied to a variety of
algorithms across many different processor architectures.

You can find out more about the design of VTK-m on our [wiki][]

Example
=======

The VTK-m source distribution includes a number of examples. The goal of the
VTK-m examples is to illustrate specific VTK-m concepts in a consistent and 
simple format. However, these examples only cover a small part of the
capabilities of VTK-m.

Below is a simple example of using VTK-m to load a VTK image file, run the
Marching Cubes algorithm on it, and render the results to an image:

```cpp
vtkm::io::reader::VTKDataSetReader reader("path/to/vtk_image_file");
inputData = reader.ReadDataSet();

vtkm::Float64 isovalue = 100.0f;
std::string fieldName = "pointvar";

// Create an isosurface filter
vtkm::filter::MarchingCubes filter;
filter.SetIsoValue(0, isovalue);
vtkm::filter::ResultDataSet result = filter.Execute( inputData,
                                                     inputData.GetField(fieldName) );
filter.MapFieldOntoOutput(result, inputData.GetField(fieldName));

// compute the bounds and extends of the input data
vtkm::Bounds coordsBounds = inputData.GetCoordinateSystem().GetBounds();
vtkm::Vec<vtkm::Float64,3> totalExtent( coordsBounds.X.Length(),
                                        coordsBounds.Y.Length(),
                                        coordsBounds.Z.Length() );
vtkm::Float64 mag = vtkm::Magnitude(totalExtent);
vtkm::Normalize(totalExtent);

// setup a camera and point it to towards the center of the input data
vtkm::rendering::Camera camera;
camera.ResetToBounds(coordsBounds);

camera.SetLookAt(totalExtent*(mag * .5f));
camera.SetViewUp(vtkm::make_Vec(0.f, 1.f, 0.f));
camera.SetClippingRange(1.f, 100.f);
camera.SetFieldOfView(60.f);
camera.SetPosition(totalExtent*(mag * 2.f));
vtkm::rendering::ColorTable colorTable("thermal");

// Create a mapper, canvas and view that will be used to render the scene
vtkm::rendering::Scene scene;
vtkm::rendering::MapperRayTracer mapper;
vtkm::rendering::CanvasRayTracer canvas(512, 512);
vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);

// Render an image of the output isosurface
vtkm::cont::DataSet& outputData = result.GetDataSet();
scene.AddActor(vtkm::rendering::Actor(outputData.GetCellSet(),
                                      outputData.GetCoordinateSystem(),
                                      outputData.GetField(fieldName),
                                      colorTable));
vtkm::rendering::View3D view(scene, mapper, canvas, camera, bg);
view.Initialize();
view.Paint();
view.SaveAs("demo_output.pnm");
```

Learning
========

VTK-m offers numerous different ways to learn how to use the provided components.
If you are interested in a high level overview of VTK-m a good place to start
is with the IEEE Vis talk ["VTK-m: Accelerating the Visualization Toolkit for Massively Threaded Architectures"](http://m.vtk.org/images/2/29/VTKmVis2016.pptx) or the older and more technical presentation
["VTK-m Overview for Intel Design Review"](http://m.vtk.org/images/a/a4/VTKmIntelMeet.pptx).

If you are interested in learning how to use the existing VTK-m codebase,
or how to integrate into your own project, we recommend reading "Part 1: Getting Started"
and "Part 2: Using VTK-m" of the [VTK-m Users Guide][].

If you want to contribute to VTK-m we recommend reading the following sections
of the [VTK-m Users Guide][].

+ "Part 2: Using VTK-m"
  - Covers the core fundamental components of VTK-m including data model, worklets, and filters.
- "Part 3: Developing with VTK-m"
  - Covers how to develop new worklets and filters.
- "Part 4: Advanced Development"
  - Covers topics such as new worklet tags, opengl interop and custom device adapters .

Contributing
============

There are many ways to contribute to [VTK-m][], with varying levels of effort.

+ Ask a question on the [VTK-m users email list](http://vtk.org/mailman/listinfo/vtkm)
+ Submit a feature request or bug, or add to an existing discussion on the VTK-m [Issue Tracker][]
+ Submit a Pull Request to improve [VTK-m]
++ See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed instructions on how to create
a Pull Request.
++ Submit an Issue or Pull Request for the [VTK-m User's Guide](http://m.vtk.org/images/c/c8/VTKmUsersGuide.pdf)

Dependencies
============

VTK-m Requires:

+  C++11 Compiler. VTK-m has been confirmed to work with the following
  + GCC 4.8+
  + Clang 3.3+
  + XCode 5.0+
  + MSVC 2013+
+  [CMake 3.3](http://www.cmake.org/download/)


Optional dependencies are:

+ CUDA Device Adapter
  +  [Cuda Toolkit 7+](https://developer.nvidia.com/cuda-toolkit)
+ TBB Device Adapter
  +  [TBB](https://www.threadingbuildingblocks.org/)
+ Rendering Module
  + The rendering module requires that you have a extension binding library  
    and one rendering library. A windowing library is not needed expect  
    for some optional tests.
  +  Extension Binding
      +  [GLEW](http://glew.sourceforge.net/)
  +  Rendering Canvas
      +  OpenGL Driver (See your GPU/iGPU vendor)
      +  EGL (See your GPU/iGPU vendor)
      +  [OSMesa](https://www.mesa3d.org/osmesa.html)
  + Windowing/Contexts
      +  EGL (See your GPU/iGPU vendor)
      +  [GLFW](http://www.glfw.org/)
      +  [GLUT](http://freeglut.sourceforge.net/)

Building
========

VTK-m supports all majors platforms ( Windows, Linux, OSX ), and uses CMake
to generate all the build rules for the project.

```
$ git clone https://gitlab.kitware.com/vtk/vtk-m.git
$ mkdir vtkm-build
$ cd vtkm-build
$ cmake-gui ../vtk-m
$ make -j<N>
$ make test
```

The VTK-m CMake configuration supports several options, including what specific
device adapters ( e.g. CUDA, TBB ) that you wish to enable. Here are some
relevant options

| Variable                    |  Description                |
|-----------------------------|-----------------------------|
| BUILD_SHARED_LIBS           | Enabled by default. Build all VTK-m libraries as shared libraries.        |
| CMAKE_BUILD_TYPE            | This statically specifies what build type (configuration) will be built in this build tree. Possible values are empty, Debug, Release, RelWithDebInfo and MinSizeRel. This variable is only meaningful to single-configuration generators (such as make and Ninja).                                                                                  |
| CMAKE_INSTALL_PREFIX        | Directory to install VTK-m into.                                          |
| VTKm_ENABLE_EXAMPLES        | Disabled by default. Turn on building of simple examples of using VTK-m.  |
| VTKm_ENABLE_BENCHMARKS      | Disabled by default. Turn on additional timing tests.                     |
| VTKm_ENABLE_CUDA            | Disabled by default. Enable CUDA backend.                                 |
| VTKm_CUDA_Architecture      | Defaults to native. Specify what GPU architecture(s) to build CUDA code for, options include native, fermi, kepler, maxwell, and pascal.                                                                       |
| VTKm_ENABLE_TBB             | Disabled by default. Enable Intel Threading Building Blocks backend.      |
| VTKm_ENABLE_TESTING         | Enabled by default. Turn on header, unit, worklet, and filter tests.      |
| VTKm_ENABLE_RENDERING       | Enabled by default. Turn on the rendering module.                         |
| VTKm_USE_64BIT_IDS          | Enabled by default. This is the size of integers used to index arrays, points, cells, etc. Use 64 bit precision when on, 32 bit precision when off.                                                             |
| VTKm_USE_DOUBLE_PRECISION   | Disabled by default. Precision to use in floating point numbers when no other precision can be inferred. Use 64 bit precision when on, 32 bit precision when off.                                        |

License
=======

VTK-m is distributed under the OSI-approved BSD 3-clause License.
See [LICENSE.txt](LICENSE.txt) for details.


[VTK-m]: https://gitlab.kitware.com/vtk/vtk-m/
[Issue Tracker]: https://gitlab.kitware.com/vtk/vtk-m/issues
[wiki]: http://m.vtk.org/
[VTK-m Users Guide]: http://m.vtk.org/images/c/c8/VTKmUsersGuide.pdf

