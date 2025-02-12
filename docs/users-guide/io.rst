==============================
File I/O
==============================

.. index::
   single: I/O
   single: file I/O

Before |Viskores| can be used to process data, data need to be loaded into the
system.
|Viskores| comes with a basic file I/O package to get started developing very
quickly.
All the file I/O classes are declared under the ``viskores::io`` namespace.

.. didyouknow::
   Files are just one of many ways to get data in and out of |Viskores|.
   In later chapters we explore ways to define |Viskores| data structures of
   increasing power and complexity.
   In particular, :secref:`dataset:Building Data Sets` describes how to build |Viskores| data set objects and Section \ref{sec:ArrayHandle:Adapting} documents how to adapt data structures defined in other libraries to be used directly in |Viskores|.

.. todo:: Add custom ArrayHandle section reference above.

------------------------------
Readers
------------------------------

.. index::
   single: file I/O; read
   single: read file

All reader classes provided by |Viskores| are located in the ``viskores::io``
namespace.
The general interface for each reader class is to accept a filename in the constructor and to provide a ``ReadDataSet`` method to load the data from disk.

The data in the file are returned in a :class:`viskores::cont::DataSet` object
as described in :chapref:`dataset:Data Sets`, but it is sufficient to know that a ``DataSet`` can be passed among readers, writers, filters, and rendering units.

Legacy VTK File Reader
==============================

Legacy VTK files are a simple open format for storing visualization data.
These files typically have a :file:`.vtk` extension.
Legacy VTK files are popular because they are simple to create and read and
are consequently supported by a large number of tools.
The format of legacy VTK files is well documented in *The VTK User's
Guide* [as well as online](https://examples.vtk.org/site/VTKFileFormats/).
Legacy VTK files can also be read and written with tools like ParaView and VisIt.

Legacy VTK files can be read using the :class:`viskores::io::VTKDataSetReader` class.

.. doxygenclass:: viskores::io::VTKDataSetReader
   :members:

.. load-example:: VTKDataSetReader
   :file: GuideExampleIO.cxx
   :caption: Reading a legacy VTK file.

Image Readers
==============================

|Viskores| provides classes to read images from some standard image formats.
These readers will store the data in a :class:`viskores::cont::DataSet` object with the colors stored as a named point field.
The colors are read as 4-component RGBA vectors for each pixel.
Each component in the pixel color is stored as a 32-bit float between 0 and 1.

Portable Network Graphics (PNG) files can be read using the :class:`viskores::io::ImageReaderPNG` class.

.. doxygenclass:: viskores::io::ImageReaderPNG
   :members:

.. load-example:: ImageReaderPNG
   :file: GuideExampleIO.cxx
   :caption: Reading an image from a PNG file.

Portable anymap (PNM) files can be read using the :class:`viskores::io::ImageReaderPNM` class.

.. doxygenclass:: viskores::io::ImageReaderPNM
   :members:

Like for PNG files, a :class:`viskores::io::ImageReaderPNM` is constructed with the name of the file to read from.

.. load-example:: ImageReaderPNM
   :file: GuideExampleIO.cxx
   :caption: Reading an image from a PNM file.


------------------------------
Writers
------------------------------

.. index::
   single: file I/O; write
   single: write file

All writer classes provided by |Viskores| are located in the ``viskores::io`` namespace.
The general interface for each writer class is to accept a filename in the constructor and to provide a ``WriteDataSet`` method to save data to the disk.
The ``WriteDataSet`` method takes a :class:`viskores::cont::DataSet` object as an argument, which contains the data to write to the file.

Legacy VTK File Writer
==============================

Legacy VTK files can be written using the :class:`viskores::io::VTKDataSetWriter` class.

.. doxygenclass:: viskores::io::VTKDataSetWriter
   :members:

.. doxygenenum:: viskores::io::FileType

.. load-example:: VTKDataSetWriter
   :file: GuideExampleIO.cxx
   :caption: Writing a legacy VTK file.

Image Writers
==============================

|Viskores| provides classes to some standard image formats.
These writers store data in a :class:`viskores::cont::DataSet`.
The data must be a 2D structure with the colors stored in a point field.
(See :chapref:`dataset:Data Sets` for details on :class:`viskores::cont::DataSet` objects.)

Portable Network Graphics (PNG) files can be written using the :class:`viskores::io::ImageWriterPNG` class.

.. doxygenclass:: viskores::io::ImageWriterPNG
   :members:

By default, PNG files are written as RGBA colors using 8-bits for each component.
You can change the format written using the :func:`viskores::io::ImageWriterPNG::SetPixelDepth` method.
This takes an item in the :enum:`viskores::io::ImageWriterPNG::PixelDepth` enumeration.

.. doxygenenum:: viskores::io::ImageWriterBase::PixelDepth

.. load-example:: ImageWriterPNG
   :file: GuideExampleIO.cxx
   :caption: Writing an image to a PNG file.

Portable anymap (PNM) files can be written using the :class:`viskores::io::ImageWriterPNM` class.

.. doxygenclass:: viskores::io::ImageWriterPNM
   :members:

.. load-example:: ImageWriterPNM
   :file: GuideExampleIO.cxx
   :caption: Writing an image to a PNM file.
