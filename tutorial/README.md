# VTK-m Tutorial

This page contains materials and instructions for the VTK-m tutorial.
[Slides] are available, and the instructions for getting the example source
code exercises are below.

Developers interested in VTK-m should also consult _[The VTK-m User's
Guide]_, which contains a quick start guide along with detailed
documentation of most of VTK-m's features.

Further information is available at https://m.vtk.org.

## Downloading VTK-m

The tutorial materials are maintained as part of the VTK-m code repository
to help keep the examples up to date. Thus, getting, compiling, and running
the tutorial examples is all part of VTK-m itself.

There are two options for getting the VTK-m source code. You could either
download a tarball for a release or you can clone the source code directly
from the [VTK-m git repository].

### Downloading a VTK-m Release Tarball

Souce code archives for every VTK-m release are posted on the [VTK-m
releases page] in multiple archive formats. Simply download an archive for
the desired version of VTK-m and extract the contents from that archive.

### Cloning the VTK-m Git Repository

Developers familiar with git might find it easier to simply clone the [VTK-m
git repository]. The latest VTK-m release is always on the `release` branch
and can be cloned as so.

```sh
git clone --branch release https://gitlab.kitware.com/vtk/vtk-m.git
```

If you are feeling more daring, you can simply clone the main branch with
the latest developments.

```sh
git clone https://gitlab.kitware.com/vtk/vtk-m.git
```

## Building VTK-m and the Tutorial Examples

To build VTK-m, you will need at a minimum CMake and, of course, a C++
compiler. The [VTK-m dependencies list] has details on required and
optional packages.

When configuring the build with CMake, turn on the `VTKm_ENABLE_TUTORIALS`
option. There are lots of other options available including the ability to
compile for many different types of devices. But if this is your first
experience with VTK-m, it might be best to start with a simple build.

Here is a list of minimal commands to download and build VTK-m.

```sh
git clone --branch release https://gitlab.kitware.com/vtk/vtk-m.git
mkdir vtk-m-build
cd vtk-m-build
cmake ../vtk-m -DVTKm_ENABLE_TUTORIALS=ON
make -j8
```

The first line above downloads VTK-m using git. You can choose to download
a release as described [above](#downloading-a-vtk-m-release-tarball). Note
that if you do so, the source code will be placed in a directory named
something like `vtk-m-vX.X.X` rather than the `vtk-m` directory you get by
default when cloning VTK-m.

## Examples

The tutorial contains several examples that can be built and edited. Each
example is described in detail in the [slides]. Here is a brief description
of each one, listed from most basic to increasing complexity.

* **io.cxx** A bare minimum example of loading a `DataSet` object from a
  data file and then writing it out again.
* **contour.cxx** A basic example of running a filter that extracts
  isosurfaces from a data set.
* **contour_two_fields.cxx** A simple extension of contour.cxx that selects
  two of the input fields to be passed to the output.
* **two_filters.cxx** Further extends the contour.cxx example by running a
  sequence of 2 filters. The first extracts the isosurfaces and the second
  clips the surface by a second fields.
* **rendering.cxx** Demonstrates how to render data in VTK-m.
* **error_handling.cxx** Demonstrates catching exceptions to react to
  errors in VTK-m execution.
* **logging.cxx** Uses VTK-m's logging mechanism to write additional
  information to the program output.
* **mag_grad.cxx** The implementation of a simple VTK-m filter which
  happens to take the magnitude of a vector.
* **point_to_cell.cxx** A slightly more complicated filter that averages
  the values of a point field for each cell.
* **extract_edges.cxx** A fully featured example of a nontrivial filter
  that extracts topological features from a mesh.

[slides]: https://www.dropbox.com/s/4pp4xf1jlvlt4th/VTKm_Tutorial_VIS22.pptx?dl=0
[The VTK-m User's Guide]: https://gitlab.kitware.com/vtk/vtk-m-user-guide/-/wikis/home
[VTK-m git repository]: https://gitlab.kitware.com/vtk/vtk-m
[VTK-m releases page]: https://gitlab.kitware.com/vtk/vtk-m/-/releases
[VTK-m dependencies list]: https://gitlab.kitware.com/vtk/vtk-m#dependencies
