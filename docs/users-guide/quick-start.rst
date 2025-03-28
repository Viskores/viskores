==============================
Quick Start
==============================

In this chapter we go through the steps to create a simple program that uses |Viskores|.
This "hello world" example presents only the bare minimum of features available.
The remainder of this book documents dives into much greater detail.

We will call the example program we are building :file:`ViskoresQuickStart`.
It will demonstrate reading data from a file, processing the data with a filter, and rendering an image of the data.
Readers who are less interested in an explanation and are more interested in browsing some code can skip to :secref:`quick-start:The Full Example`.

------------------------------
Initialize
------------------------------

.. index:: initialization

The first step to using |Viskores| is to initialize the library.
Although initializing |Viskores| is *optional*, it is recommend to allow |Viskores| to configure devices and logging.
Initialization is done by calling the :func:`viskores::cont::Initialize` function.
The ``Initialize`` function is defined in the :file:`viskores/cont/Initialize.h` header file.

``Initialize`` takes the ``argc`` and ``argv`` arguments that are passed to the ``main`` function of your program, find any command line arguments relevant to |Viskores|, and remove them from the list to make further command line argument processing easier.

.. load-example:: ViskoresQuickStartInitialize
   :file: ViskoresQuickStart.cxx
   :caption: Initializing |Viskores|.

``Initialize`` has many options to customize command line argument processing.
See :chapref:`initialization:Initialization` for more details.

.. didyouknow::
  Don't have access to ``argc`` and ``argv``?
  No problem.
  You can call :func:`viskores::cont::Initialize` with no arguments.


------------------------------
Reading a File
------------------------------

.. index::
   single: file I/O ; read
   single: read file

|Viskores| comes with a simple I/O library that can read and write files in VTK legacy format.
These files have a :file:`.vtk` extension.

VTK legacy files can be read using the :class:`viskores::io::VTKDataSetReader` object, which is declared in the :file:`viskores/io/VTKDataSetReader.h` header file.
The object is constructed with a string specifying the filename (which for this example we will get from the command line).
The data is then read in by calling the :func:`viskores::io::VTKDataSetReader::ReadDataSet` method.

.. load-example:: ViskoresQuickStartReadFile
   :file: ViskoresQuickStart.cxx
   :caption: Reading data from a VTK legacy file.

The ``ReadDataSet`` method returns the data in a :class:`viskores::cont::DataSet` object.
The structure and features of a ``DataSet`` object is described in :chapref:`dataset:Data Sets`.
For the purposes of this quick start, we will treat ``DataSet`` as a mostly opaque object that gets passed to and from operations in |Viskores|.

More information about |Viskores|'s file readers and writers can be found in :chapref:`io:File I/O`.


------------------------------
Running a Filter
------------------------------

.. index:: filter

Algorithms in |Viskores| are encapsulated in units called *filters*.
A filter takes in a ``DataSet``, processes it, and returns a new ``DataSet``.
The returned ``DataSet`` often, but not always, contains data inherited from the source data.

|Viskores| comes with many filters, which are documented in :chapref:`provided-filters:Provided Filters`.
For this example, we will demonstrate the use of the :class:`viskores::filter::MeshQuality` filter, which is defined in the :file:`viskores/filter/MeshQuality.h` header file.
The ``MeshQuality`` filter will compute for each cell in the input data will compute a quantity representing some metric of the cell's shape.
Several metrics are available, and in this example we will find the area of each cell.

Like all filters, ``MeshQuality`` contains an ``Execute`` method that takes an input ``DataSet`` and produces an output ``DataSet``.
It also has several methods used to set up the parameters of the execution.
:secref:`provided-filters:Mesh Quality Metrics` provides details on all the options of ``MeshQuality``.
Suffice it to say that in this example we instruct the filter to find the area of each cell, which it will output to a field named ``area``.

.. load-example:: ViskoresQuickStartFilter
   :file: ViskoresQuickStart.cxx
   :caption: Running a filter.


------------------------------
Rendering an Image
------------------------------

.. index:: rendering

Although it is possible to leverage external rendering systems, |Viskores| comes with its own self-contained image rendering algorithms.
These rendering classes are completely implemented with the parallel features provided by |Viskores|, so using rendering in |Viskores| does not require any complex library dependencies.

Even a simple rendering scene requires setting up several parameters to establish what is to be featured in the image including what data should be rendered, how that data should be represented, where objects should be placed in space, and the qualities of the image to generate.
Consequently, setting up rendering in |Viskores| involves many steps.
:chapref:`rendering:Rendering` goes into much detail on the ways in which a rendering scene is specified.
For now, we just briefly present some boilerplate to achieve a simple rendering.

.. load-example:: ViskoresQuickStartRender
   :file: ViskoresQuickStart.cxx
   :caption: Rendering data.

.. index::
   single: scene
   single: actor

The first step in setting up a render is to create a *scene*.
A scene comprises some number of *actors*, which represent some data to be rendered in some location in space.
In our case we only have one ``DataSet`` to render, so we simply create a single actor and add it to a scene as shown in :exlineref:`Example {number} lines {line}<ViskoresQuickStartRender:scene-start>` :exlineref:`-- %s<ViskoresQuickStartRender:scene-end>`.

.. index::
   single: view
   single: mapper
   single: canvas

The second step in setting up a render is to create a *view*.
The view comprises the aforementioned scene, a *mapper*, which describes how the data are to be rendered, and a *canvas*, which holds the image buffer and other rendering context.
The view is created in :exlineref:`ViskoresQuickStartRender:view`.
The image generation is then performed by calling :func:`viskores::rendering::View::Paint` on the view object (:exlineref:`ViskoresQuickStartRender:paint`).
However, the rendering done by |Viskores|'s rendering classes is performed offscreen, which means that the result does not appear on your computer's monitor.
The easiest way to see the image is to save it to an image file using the :func:`viskores::rendering::View::SaveAs` method (:exlineref:`ViskoresQuickStartRender:save`).


------------------------------
The Full Example
------------------------------

Putting together the examples from the previous sections, here is a complete program for reading, processing, and rendering data with |Viskores|.

.. load-example:: ViskoresQuickStart
   :file: ViskoresQuickStart.cxx
   :caption: Simple example of using |Viskores|.


------------------------------
Build Configuration
------------------------------

.. index:: CMakeLists.txt

Now that we have the program listed in :numref:`ex:ViskoresQuickStart`, we still need to compile it with the appropriate compilers and flags.
By far the easiest way to compile |Viskores| code is to use CMake.
CMake commands that can be used to link code to |Viskores| are discussed in :secref:`building:Linking to |Viskores|`.
The following example provides a minimal :file:`CMakeLists.txt` required to build this program.

.. load-example:: QuickStartCMakeLists.txt
   :file: ViskoresQuickStart.cmake
   :caption: :file:`CMakeLists.txt` to build a program using |Viskores|.
   :language: cmake
   :command-comment: ####

The first two lines contain boilerplate for any :file:`CMakeLists.txt` file.
They all should declare the minimum CMake version required (for backward compatibility) and have a :cmake:command:`project` command to declare which languages are used.

The remainder of the commands find the |Viskores| library, declare the program begin compiled, and link the program to the |Viskores| library.
These steps are described in detail in :secref:`building:Linking to |Viskores|`.
