# Viskores Tutorial

This page contains materials and instructions for the Viskores tutorial.
[Slides] are available, and the instructions for getting the example source
code exercises are below.

Developers interested in Viskores should also consult _[The Viskores User's
Guide]_, which contains a quick start guide along with detailed
documentation of most of Viskores's features.

Further information is available at https://m.vtk.org.

## Downloading Viskores

The tutorial materials are maintained as part of the Viskores code repository
to help keep the examples up to date. Thus, getting, compiling, and running
the tutorial examples is all part of Viskores itself.

There are two options for getting the Viskores source code. You could either
download a tarball for a release or you can clone the source code directly
from the [Viskores git repository].

### Downloading a Viskores Release Tarball

Souce code archives for every Viskores release are posted on the [Viskores
releases page] in multiple archive formats. Simply download an archive for
the desired version of Viskores and extract the contents from that archive.

### Cloning the Viskores Git Repository

Developers familiar with git might find it easier to simply clone the [Viskores
git repository]. The latest Viskores release is always on the `release` branch
and can be cloned as so.

```sh
git clone --branch release https://gitlab.kitware.com/vtk/viskores.git
```

If you are feeling more daring, you can simply clone the main branch with
the latest developments.

```sh
git clone https://gitlab.kitware.com/vtk/viskores.git
```

## Building Viskores and the Tutorial Examples

To build Viskores, you will need at a minimum CMake and, of course, a C++
compiler. The [Viskores dependencies list] has details on required and
optional packages.

When configuring the build with CMake, turn on the `Viskores_ENABLE_TUTORIALS`
option. There are lots of other options available including the ability to
compile for many different types of devices. But if this is your first
experience with Viskores, it might be best to start with a simple build.

Here is a list of minimal commands to download and build Viskores.

```sh
git clone --branch release https://gitlab.kitware.com/vtk/viskores.git
mkdir viskores-build
cd viskores-build
cmake ../viskores -DViskores_ENABLE_TUTORIALS=ON
make -j8
```

The first line above downloads Viskores using git. You can choose to download
a release as described [above](#downloading-a-viskores-release-tarball). Note
that if you do so, the source code will be placed in a directory named
something like `viskores-vX.X.X` rather than the `viskores` directory you get by
default when cloning Viskores.

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
* **rendering.cxx** Demonstrates how to render data in Viskores.
* **error_handling.cxx** Demonstrates catching exceptions to react to
  errors in Viskores execution.
* **logging.cxx** Uses Viskores's logging mechanism to write additional
  information to the program output.
* **mag_grad.cxx** The implementation of a simple Viskores filter which
  happens to take the magnitude of a vector.
* **point_to_cell.cxx** A slightly more complicated filter that averages
  the values of a point field for each cell.
* **extract_edges.cxx** A fully featured example of a nontrivial filter
  that extracts topological features from a mesh.

[slides]: https://www.dropbox.com/s/4pp4xf1jlvlt4th/Viskores_Tutorial_VIS22.pptx?dl=0
[The Viskores User's Guide]: https://gitlab.kitware.com/vtk/viskores-user-guide/-/wikis/home
[Viskores git repository]: https://gitlab.kitware.com/vtk/viskores
[Viskores releases page]: https://gitlab.kitware.com/vtk/viskores/-/releases
[Viskores dependencies list]: https://gitlab.kitware.com/vtk/viskores#dependencies
