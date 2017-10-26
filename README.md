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

  + Community discussion takes place on the [VTK-m users email list].

  + Doxygen-generated nightly reference documentation is available
    [online][VTK-m Doxygen].


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
      + GCC 4.8+
      + Clang 3.3+
      + XCode 5.0+
      + MSVC 2013+
  + [CMake](http://www.cmake.org/download/)
      + CMake 3.3+ (for any build)
      + CMake 3.9+ (for CUDA build)

Optional dependencies are:

  + CUDA Device Adapter
      + [Cuda Toolkit 7+](https://developer.nvidia.com/cuda-toolkit)
  + TBB Device Adapter
      + [TBB](https://www.threadingbuildingblocks.org/)
  + OpenGL Rendering
      + The rendering module contains multiple rendering implementations
        including standalone rendering code. The rendering module also
        includes (optionally built) OpenGL rendering classes.
      + The OpenGL rendering classes require that you have a extension
        binding library and one rendering library. A windowing library is
        not needed expect for some optional tests.
  + Extension Binding
      + [GLEW](http://glew.sourceforge.net/)
  + Rendering Canvas
      + OpenGL Driver (See your GPU/iGPU vendor)
  + Windowing/Contexts
      + EGL (See your GPU/iGPU vendor)
      + [OSMesa](https://www.mesa3d.org/osmesa.html)
      + [GLFW](http://www.glfw.org/)
      + [GLUT](http://freeglut.sourceforge.net/)


## Building ##

VTK-m supports all majors platforms (Windows, Linux, OSX), and uses CMake
to generate all the build rules for the project. The VTK-m source code is
available from the [VTK-m download page] or by directly cloning the [VTK-m
git repository].

```
$ git clone https://gitlab.kitware.com/vtk/vtk-m.git
$ mkdir vtkm-build
$ cd vtkm-build
$ cmake-gui ../vtk-m
$ make -j<N>
$ make test
```

A more detailed description of building VTK-m is available in the [VTK-m
Users Guide].


## License ##

VTK-m is distributed under the OSI-approved BSD 3-clause License.
See [LICENSE.txt](LICENSE.txt) for details.


[VTK-m]:                    https://gitlab.kitware.com/vtk/vtk-m/
[VTK-m Coding Conventions]: docs/CodingConventions.md
[VTK-m Doxygen]:            http://m.vtk.org/documentation/
[VTK-m download page]:      http://m.vtk.org/index.php/VTK-m_Releases
[VTK-m git repository]:     https://gitlab.kitware.com/vtk/vtk-m/
[VTK-m Issue Tracker]:      https://gitlab.kitware.com/vtk/vtk-m/issues
[VTK-m Overview]:           http://m.vtk.org/images/2/29/VTKmVis2016.pptx 
[VTK-m Users Guide]:        http://m.vtk.org/images/c/c8/VTKmUsersGuide.pdf
[VTK-m users email list]:   http://vtk.org/mailman/listinfo/vtkm
[VTK-m Wiki]:               http://m.vtk.org/
[CONTRIBUTING.md]:          CONTRIBUTING.md
