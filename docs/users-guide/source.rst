==============================
Sources
==============================

.. index::
   single: source
   single: data set; source

Before |Viskores| can process data, data must be placed in a
:class:`viskores::cont::DataSet` or
:class:`viskores::cont::PartitionedDataSet`.
Files are one common way to get data into |Viskores|, as described in
:chapref:`io:File I/O`.
|Viskores| also provides a set of *source* classes that generate synthetic data.
These sources are useful for examples, tests, benchmarks, and for quickly
constructing input to filters without managing external files.

All source classes provided by |Viskores| are located in the
``viskores::source`` namespace and declared in headers under
:file:`viskores/source`.
Most sources derive from :class:`viskores::source::Source`.
The general interface is to construct a source, set any desired parameters, and
call ``Execute`` to generate the data.

.. doxygenclass:: viskores::source::Source
   :members:

------------------------------
Uniform Grid Sources
------------------------------

Several source classes generate structured 3D data sets with uniform point
coordinates.
Most of these sources provide ``SetPointDimensions`` and ``SetCellDimensions`` methods.
Point dimensions specify the number of points in each direction.
Cell dimensions specify the number of cells in each direction, and the source
will create one more point than cell in each direction.

Tangle
==============================

.. index::
   single: source; Tangle
   single: Tangle

The :class:`viskores::source::Tangle` source creates a uniform structured data
set over the unit cube.
It adds a scalar point field named ``tangle``.
The field is computed from a polynomial with a smooth, rounded shape that is
useful for contouring, clipping, and other scalar-field operations.
The ``tangle`` field usually ranges from just below 0 to about 25, with the
most interesting contour surfaces near the lower end of that range.
Good starting contour values are 0, 0.5, 1, and 2.

.. doxygenclass:: viskores::source::Tangle
   :members:

.. load-example:: SourceTangle
   :file: GuideExampleSource.cxx
   :caption: Creating a tangle data set.

Perlin Noise
==============================

.. index::
   single: source; PerlinNoise
   single: Perlin noise

The :class:`viskores::source::PerlinNoise` source creates a uniform structured
data set with a scalar point field named ``perlinnoise``.
The field contains tileable Perlin noise, which provides a smooth random-looking
field for testing algorithms on noisy data.
The ``perlinnoise`` field is normalized to values typically between 0 and 1.
Contours at 0.3, 0.4, 0.5, 0.6, and 0.7 usually provide a useful spread of
features.

If a seed is set with
:func:`viskores::source::PerlinNoise::SetSeed`, repeated executions produce the
same field values.
If no seed is set, a new seed is selected each time ``Execute`` is called.
The origin of the generated coordinate system can be changed with
:func:`viskores::source::PerlinNoise::SetOrigin`.

.. doxygenclass:: viskores::source::PerlinNoise
   :members:

.. load-example:: SourcePerlinNoise
   :file: GuideExampleSource.cxx
   :caption: Creating a deterministic Perlin noise data set.

Oscillator
==============================

.. index::
   single: source; Oscillator
   single: oscillator source

The :class:`viskores::source::Oscillator` source creates a time-varying uniform
structured data set.
It adds a scalar point field named ``oscillating``.
The field is formed from one or more Gaussian oscillator terms.
The oscillators are configured by adding periodic, damped, or decaying
oscillators and by setting the current time.

Each oscillator is specified by a center point, radius, angular frequency, and
damping factor.
The generated coordinate system spans the unit cube.
The range of the ``oscillating`` field depends on the oscillators that have
been added and the selected time.
With a small number of moderate oscillators, values often fall near -1 to 1.
For contouring, inspect the generated field range and start with values around
0, or with several values symmetrically distributed around 0 such as -0.5, 0,
and 0.5.

.. doxygenclass:: viskores::source::Oscillator
   :members:

.. load-example:: SourceOscillator
   :file: GuideExampleSource.cxx
   :caption: Creating a time-varying oscillator data set.

Wavelet Source
==============================

.. index::
   single: source; Wavelet
   single: wavelet source
   single: RTData

The :class:`viskores::source::Wavelet` source creates a structured data set
similar to VTK's ``vtkRTAnalyticSource``.
It adds a scalar point field named ``RTData``.
This source is useful for examples that need a smooth scalar field with
nontrivial variation throughout the domain.

The geometry of the generated data set is controlled by the logical point
extent, spacing, and origin.
The source creates points for every logical index between the minimum and
maximum extent values, inclusive.
If the z extent has zero length, the generated data set uses a 2D structured
cell set; otherwise it uses a 3D structured cell set.

The scalar field is controlled by parameters such as center, standard
deviation, maximum value, frequency, and magnitude.
By default the extent is ``{-10, -10, -10}`` to ``{10, 10, 10}``, spacing is
``{1, 1, 1}``, and the field center is at ``{0, 0, 0}``.
With the default parameters, the ``RTData`` field is typically between about
40 and 280.
Useful contour values generally include 100, 150, 200, and 250.

.. doxygenclass:: viskores::source::Wavelet
   :members:

.. load-example:: SourceWavelet
   :file: GuideExampleSource.cxx
   :caption: Creating a 3D wavelet data set.

.. load-example:: SourceWavelet2D
   :file: GuideExampleSource.cxx
   :caption: Creating a 2D wavelet data set by using a zero-length z extent.

------------------------------
AMR Source
------------------------------

.. index::
   single: source; Amr
   single: AMR
   single: PartitionedDataSet; source

The :class:`viskores::source::Amr` source creates a
:class:`viskores::cont::PartitionedDataSet` that represents a simple adaptive
mesh refinement hierarchy.
Unlike the other source classes in this chapter, ``Amr`` does not derive from
:class:`viskores::source::Source` because its ``Execute`` method returns a
partitioned data set rather than a single data set.

Each partition is generated from a wavelet source and contains a point field
named ``RTData`` and a cell field named ``RTDataCells``.
The source also runs :class:`viskores::filter::multi_block::AmrArrays` to add
helper arrays used by AMR-aware filters.

The source can generate either 2D or 3D AMR data.
The number of cells per dimension must be even and greater than 1.
The number of levels controls how many refinement levels are generated; level
``l`` contains ``2^l`` partitions.
Because the AMR source uses wavelet data for each partition, the point field
``RTData`` and cell field ``RTDataCells`` usually have values in roughly the
same range as the wavelet source, about 40 to 285 for common parameters.
Contours at 100, 150, 200, and 250 are good starting values for exploring the
generated hierarchy.

.. doxygenclass:: viskores::source::Amr
   :members:

.. load-example:: SourceAmr
   :file: GuideExampleSource.cxx
   :caption: Creating an AMR partitioned data set.
