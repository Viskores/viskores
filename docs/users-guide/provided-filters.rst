------------------------------
Provided Filters
------------------------------

|VTKm| comes with the implementation of many filters.
Filters in |VTKm| are divided into a collection of modules, each with its own namespace and library.
This section is organized by each filter module, each of which contains one or more filters that are related to each other.

Note that this is not an exhaustive list of filters available in |VTKm|.
More can be found in the namespaces under ``vtkm::filter`` (and likewise the subdirectories under :file:`vtkm/filter` in the |VTKm| source.

..
   Common filter methods:
   SetActiveField, GetActiveField, SetUseCoordinateSystemAsField, GetUseCoordinateSystemAsField, SetActiveCoordinateSystem, GetActiveCoordinateSystem, SetOutputFieldName, GetOutputFieldName, Execute

..
   % These commands are used in the bottom of a description environment used for methods on filters. All should provide whichever ones make the most sense. All these commands take an optional argument that has a list of methods to supress (i.e. _not_ document) for those that are not relevant to the filter or should be documented in a different way.

   % This has the base methods available on all filters.
   \NewDocumentCommand{\commonfiltermethods}{O{}}{
     \IfSubStr{#1}{Execute}{}{
     \item[\textcode{Execute}]
       Takes a data set, executes the filter on a device, and returns a data set that contains the result.
     }
     \IfSubStr{#1}{FieldsToPass}{}{
     \item[\textcode{SetFieldsToPass}/\textcode{GetFieldsToPass}]
       Specifies which fields to pass from input to output.
       By default all fields are passed.
       See Section~\ref{sec:FilterPassingFields} for more details.
     }
   }

   \NewDocumentCommand{\commonfieldfiltermethods}{O{}}{
     \IfSubStr{#1}{ActiveField}{}{
     \item[\textcode{SetActiveField}/\textcode{GetActiveFieldName}]
       Specifies the name of the field to use as input.
     }
     \IfSubStr{#1}{UseCoordinateSystemAsField}{}{
     \item[\textcode{SetUseCoordinateSystemAsField}/\textcode{GetUseCoordinateSystemAsField}]
       Specifies a Boolean flag that determines whether to use point coordinates as the input field.
       Set to false by default.
       When true, the values for the active field are ignored and the active coordinate system is used instead.
     }
     \IfSubStr{#1}{ActiveCoordinateSystem}{}{
     \item[\textcode{SetActiveCoordinateSystem}/\textcode{GetActiveCoordinateSystemIndex}]
       Specifies the index of which coordinate system to use as the input field.
       The default index is 0, which is the first coordinate system.
     }
     \IfSubStr{#1}{OutputFieldName}{}{
     \item[\textcode{SetOutputFieldName}/\textcode{GetOutputFieldName}]
       Specifies the name of the output field generated.
     }
     \commonfiltermethods[#1]
   }

   % Obsolete in new filter structure (use \commonfiltermethods)
   \NewDocumentCommand{\commondatasetfiltermethods}{O{}}{
     \IfSubStr{#1}{ActiveCoordinateSystem}{}{
     \item[\textcode{SetActiveCoordinateSystem}/\textcode{GetActiveCoordinateSystemIndex}]
       Specifies the index of which coordinate system to use as when computing spatial locations in the mesh.
       The default index is 0, which is the first coordinate system.
     }
     \commonfiltermethods[#1]
   }

   % Obsolete in new filter structure (use \commonfieldfiltermethods)
   \NewDocumentCommand{\commondatasetwithfieldfiltermethods}{O{}}{
     \IfSubStr{#1}{ActiveField}{}{
     \item[\textcode{SetActiveField}/\textcode{GetActiveFieldName}]
       Specifies the name of the field to use as input.
     }
     \IfSubStr{#1}{UseCoordinateSystemAsField}{}{
     \item[\textcode{SetUseCoordinateSystemAsField}/\textcode{GetUseCoordinateSystemAsField}]
       Specifies a Boolean flag that determines whether to use point coordinates as the input field.
       Set to false by default.
       When true, the values for the active field are ignored.
     }
     \IfSubStr{#1}{ActiveCoordinateSystem}{}{
     \item[\textcode{SetActiveCoordinateSystem}/\textcode{GetActiveCoordinateSystemIndex}]
       Specifies the index of which coordinate system to use as when computing spatial locations in the mesh.
       The default index is 0, which is the first coordinate system.
     }
     \commonfiltermethods[#1]
   }


Cleaning Grids
==============================

.. index::
   double: clean grid; filter
   single: data set; clean

The ``vtkm::filter::clean_grid`` module contains filters that resolve issues with mesh structure.
This could include finding and merging coincident points, removing degenerate cells, or converting the grid to a known type.

Clean Grid
------------------------------

:class:`vtkm::filter::clean_grid::CleanGrid` is a filter that converts a cell set to an explicit representation and potentially removes redundant or unused data.
It does this by iterating over all cells in the data set, and for each one creating the explicit cell representation that is stored in the output.
(Explicit cell sets are described in :secref:`dataset:Explicit Cell Sets`.)
One benefit of using :class:`vtkm::filter::clean_grid::CleanGrid` is that it can optionally remove unused points and combine coincident points.
Another benefit is that the resulting cell set will be of a known specific type.

.. commonerrors::
  The result of :class:`vtkm::filter::clean_grid::CleanGrid` is not necessarily smaller, memory-wise, than its input.
  For example, "cleaning" a data set with a structured topology will actually result in a data set that requires much more memory to store an explicit topology.

.. doxygenclass:: vtkm::filter::clean_grid::CleanGrid
   :members:

Connected Components
==============================

.. index::
   double: connected components; filter

Connected components in a mesh are groups of mesh elements that are connected together in some way.
For example, if two cells are neighbors, then they are in the same component.
Likewise, a cell is also in the same component as its neighbor's neighbors as well as their neighbors and so on.
Connected components help identify when features in a simulation fragment or meld.

The ``vtkm::filter::connected_components`` module contains filters that find groups of cells that are connected.
There are different ways to define what it means to be connected.
One way is to use the topological connections of the cells.
That is, two cells that share a point, edge, or face are connected.
Another way is to use a field that classifies each cell, and cells are only connected if they have the same classification.

Cell Connectivity
------------------------------

.. index:: connected components; cell

The :class:`vtkm::filter::connected_components::CellSetConnectivity` filter finds groups of cells that are connected together through their topology.

.. doxygenclass:: vtkm::filter::connected_components::CellSetConnectivity
   :members:

Classification Field on Image Data
----------------------------------------

.. index::
   double: connected components; image
   double: connected components; field
   double: connected components; filter

The :class:`vtkm::filter::connected_components::ImageConnectivity` filter finds groups of points that have the same field value and are connected together through their topology.

.. doxygenclass:: vtkm::filter::connected_components::ImageConnectivity
   :members:


Contouring
==============================

.. index:: double: contouring; filter

The ``vtkm::filter::contour`` module contains filters that extract regions that match some field or spatial criteria.
Unlike :numref:`entity extraction filters (Section %s)<provided-filters:Entity Extraction>`, the geometry will be clipped or sliced to extract the exact matching region.
(In contrast, entity extraction filters will pull unmodified points, edges, faces, or cells from the input.)

Contour
------------------------------

.. index::
   double: contour; filter
   double: isosurface; filter

*Contouring* is one of the most fundamental filters in scientific visualization.
A contour is the locus where a field is equal to a particular value.
A topographic map showing curves of various elevations often used when hiking in hilly regions is an example of contours of an elevation field in 2 dimensions.
Extended to 3 dimensions, a contour gives a surface.
Thus, a contour is often called an *isosurface*.
The contouring/isosurface algorithm is implemented by :class:`vtkm::filter::contour::Contour`.

.. doxygenclass:: vtkm::filter::contour::Contour
   :members:

:class:`vtkm::filter::contour::Contour` also inherits the following methods.

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::SetIsoValue(vtkm::Float64)

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::SetIsoValue(vtkm::Id, vtkm::Float64)

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::SetIsoValues

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::GetIsoValue

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::SetGenerateNormals

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::GetGenerateNormals

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::SetComputeFastNormals

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::GetComputeFastNormals

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::SetNormalArrayName

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::GetNormalArrayName

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::SetMergeDuplicatePoints

.. doxygenfunction:: vtkm::filter::contour::AbstractContour::GetMergeDuplicatePoints

.. load-example:: Contour
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`vtkm::filter::contour::Contour`.

Slice
------------------------------

.. index::
   double: slice; filter

A slice operation intersects a mesh with a surface.
The :class:`vtkm::filter::contour::Slice` filter uses a :class:`vtkm::ImplicitFunctionGeneral` to specify an implicit surface to slice on.
A plane is a common thing to slice on, but other surfaces are available.
See :chapref:`implicit-functions:Implicit Functions` for information on implicit functions.

.. doxygenclass:: vtkm::filter::contour::Slice
   :members:

The :class:`vtkm::filter::contour::Slice` filter inherits from the :class:`vtkm::filter::contour::Contour`, uses its implementation to extract the slices, and several of the inherited methods are useful including :func:`vtkm::filter::contour::AbstractContour::SetGenerateNormals`, :func:`vtkm::filter::contour::AbstractContour::GetGenerateNormals`, :func:`vtkm::filter::contour::AbstractContour::SetComputeFastNormals`, :func:`vtkm::filter::contour::AbstractContour::GetComputeFastNormals`, :func:`vtkm::filter::contour::AbstractContour::SetNormalArrayName`, :func:`vtkm::filter::contour::AbstractContour::GetNormalArrayName`, :func:`vtkm::filter::contour::AbstractContour::SetMergeDuplicatePoints`, :func:`vtkm::filter::contour::AbstractContour::GetMergeDuplicatePoints`, :func:`vtkm::filter::Field::SetActiveCoordinateSystem`, and :func:`vtkm::filter::Field::GetActiveCoordinateSystemIndex`.

Clip with Field
------------------------------

.. index::
   double: clip; filter
   double: clip; field
   single: isovolume
   single: interval volume

Clipping is an operation that removes regions from the data set based on a user-provided value or function.
The :class:`vtkm::filter::contour::ClipWithField` filter takes a clip value as an argument and removes regions where a named scalar field is below (or above) that value.
(A companion filter that discards a region of the data based on an implicit function is described later.)

The result of :class:`vtkm::filter::contour::ClipWithField` is a volume.
If a cell has field values at its vertices that are all below the specified value, then it will be discarded entirely.
Likewise, if a cell has field values at its vertices that are all above the specified value, then it will be retained in its entirety.
If a cell has some vertices with field values below the specified value and some above, then the cell will be split into the portions above the value (which will be retained) and the portions below the value (which will be discarded).

This operation is sometimes called an *isovolume* because it extracts the volume of a mesh that is inside the iso-region of a scalar.
This is in contrast to an *isosurface*, which extracts only the surface of that iso-value.
That said, a more appropriate name is *interval volume* as the volume is defined by a range of values, not a single "iso" value.

:class:`vtkm::filter::contour::ClipWithField` is also similar to a threshold operation, which extracts cells based on the value of field.
The difference is that threshold will either keep or remove entire cells based on the field values whereas clip with carve cells that straddle the valid regions.
See :secref:`provided-filters:Threshold` for information on threshold extraction.

.. doxygenclass:: vtkm::filter::contour::ClipWithField
   :members:

.. load-example:: ClipWithField
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`vtkm::filter::contour::ClipWithField`.

Clip with Implicit Function
------------------------------

.. index::
   double: clip; filter
   double: clip; implicit function

The :class:`vtkm::filter::contour::ClipWithImplicitFunction` function takes an implicit function and removes all parts of the data that are inside (or outside) that function.
See :chapref:`implicit-functions:Implicit Functions` for more detail on how implicit functions are represented in |VTKm|.
A companion filter that discards a region of the data based on the value of a scalar field is described in :secref:`provided-filters:Extract Geometry`.

The result of :class:`vtkm::filter::contour::ClipWithImplicitFunction` is a volume.
If a cell has its vertices positioned all outside the implicit function, then it will be discarded entirely.
Likewise, if a cell its vertices all inside the implicit function, then it will be retained in its entirety.
If a cell has some vertices inside the implicit function and some outside, then the cell will be split into the portions inside (which will be retained) and the portions outside (which will be discarded).

.. doxygenclass:: vtkm::filter::contour::ClipWithImplicitFunction
   :members:

In the example provided below the :class:`vtkm::Sphere` implicit function is used.
This function evaluates to a negative value if points from the original dataset occur within the sphere, evaluates to 0 if the points occur on the surface of the sphere, and evaluates to a positive value if the points occur outside the sphere.

.. load-example:: ClipWithImplicitFunction
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`vtkm::filter::contour::ClipWithImplicitFunction`.


Density Estimation
==============================

.. index::
   double: density; filter

Density estimation takes a collection of samples and estimates the density of the samples in each part of the domain (or estimate the probabilty that a sample would be at a location in the domain).
The domain of samples could be a physical space, such as with particle density, or in an abstract place, such as with a histogram.
The ``vtkm::filter::density_estimate`` module contains filters that estimate density in a variety of ways.

.. todo:: Entropy, NDEntropy, and NDHistogram filters are not documented.

Histogram
------------------------------

.. index::
   double: histogram; filter
   double: density; histogram

The :class:`vtkm::filter::density_estimate::Histogram` filter computes a histogram of a given scalar field.

.. doxygenclass:: vtkm::filter::density_estimate::Histogram
   :members:

Particle Density
------------------------------

|VTKm| provides multiple filters to take as input a collection of points and build a regular mesh containing an estimate of the density of particles in that space. These filters inhert from :class:`vtkm::filter::density_estimate::ParticleDensityBase`.

.. doxygenclass:: vtkm::filter::density_estimate::ParticleDensityBase
   :members:

Nearest Grid Point
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. index::
   triple: particle; density; nearest grid point

The :class:`vtkm::filter::density_estimate::ParticleDensityNearestGridPoint` filter defines a 3D grid of bins.
It then takes from the input a collection of particles, identifies which bin each particle lies in, and sums some attribute from a field of the input (or the particles can simply be counted).

.. doxygenclass:: vtkm::filter::density_estimate::ParticleDensityNearestGridPoint
   :members:

Cloud in Cell
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. index::
   triple: particle; density; cloud in cell

The :class:`vtkm::filter::density_estimate::ParticleDensityCloudInCell` filter defines a 3D grid of bins.
It then takes from the input a collection of particles, identifies which bin each particle lies in, and then redistributes each particle's attribute to the 8 vertices of the containing bin.
The filter then sums up all the contributions of particles for each bin in the grid.

.. doxygenclass:: vtkm::filter::density_estimate::ParticleDensityCloudInCell
   :members:

Statistics
------------------------------

Simple descriptive statics for data in field arrays can be computed with :class:`vtkm::filter::density_estimate::Statistics`.

.. doxygenclass:: vtkm::filter::density_estimate::Statistics
   :members:

Entity Extraction
==============================

.. index::
   double: filter; entity extraction

|VTKm| contains a collection of filters that extract a portion of one :class:`vtkm::cont::DataSet` and construct a new :class:`vtkm::cont::DataSet` based on that portion of the geometry.
These filters are collected in the ``vtkm::filter::entity_extraction`` module.

External Faces
------------------------------

.. index::
   double: external faces; filter
   single: face; external

:class:`vtkm::filter::entity_extraction::ExternalFaces` is a filter that extracts all the external faces from a polyhedral data set.
An external face is any face that is on the boundary of a mesh.
Thus, if there is a hole in a volume, the boundary of that hole will be considered external.
More formally, an external face is one that belongs to only one cell in a mesh.

.. doxygenclass:: vtkm::filter::entity_extraction::ExternalFaces
   :members:

Extract Geometry
------------------------------

.. index::
   double: extract geometry; filter

The :class:`vtkm::filter::entity_extraction::ExtractGeometry` filter extracts all of the cells in a :class:`vtkm::cont::DataSet` that is inside or outside of an implicit function.
Implicit functions are described in :chapref:`implicit-functions:Implicit Functions`.
They define a function in 3D space that follow a geometric shape.
The inside of the implicit function is the region of negative values.

.. doxygenclass:: vtkm::filter::entity_extraction::ExtractGeometry
   :members:

Extract Points
------------------------------

.. index::
   double: extract points; filter

The :class:`vtkm::filter::entity_extraction::ExtractPoints` filter behaves the same as :class:`vtkm::filter::entity_extraction::ExtractGeometry` (:numref:`Section %s<provided-filters:Extract Geometry>`) except that the geometry is converted into a point cloud.
The filter determines whether each point is inside or outside the implicit function and passes only those that match the criteria.
The cell information of the input is thrown away and replaced with a cell set of "vertex" cells, one per point.

.. doxygenclass:: vtkm::filter::entity_extraction::ExtractPoints
   :members:

Extract Structured
------------------------------

.. index::
   double: extract structured; filter

:class:`vtkm::filter::entity_extraction::ExtractStructured` is a filter that extracts a volume of interest (VOI) from a structured data set.
In addition the filter is able to subsample the VOI while doing the extraction.
The input and output of this filter are a structured data sets.

.. doxygenclass:: vtkm::filter::entity_extraction::ExtractStructured
   :members:

Ghost Cell Removal
------------------------------

.. index::
   double: ghost cell; filter
   single: ghost cell; remove
   single: blanked cell; remove

The :class:`vtkm::filter::entity_extraction::GhostCellRemove` filter is used to remove cells from a data set according to a cell centered field that specifies whether a cell is a regular cell or a ghost cell.
By default, the filter will get the ghost cell information that is registered in the input :class:`vtkm::cont::DataSet`, but it also possible to specify an arbitrary field for this purpose.

.. todo:: Better document how ghost cells work in |VTKm| (somewhere).

.. doxygenclass:: vtkm::filter::entity_extraction::GhostCellRemove
   :members:

Threshold
------------------------------

.. index::
   double: threshold; filter

A threshold operation removes topology elements from a data set that do not meet a specified criterion.
The :class:`vtkm::filter::entity_extraction::Threshold` filter removes all cells where the a field is outside a range of values.

Note that :class:`vtkm::filter::entity_extraction::Threshold` either passes an entire cell or discards an entire cell.
This can consequently lead to jagged surfaces at the interface of the threshold caused by the shape of cells that jut inside or outside the removed region.
See :secref:`provided-filters:Clip with Field` for a clipping filter that will clip off a smooth region of the mesh.

.. doxygenclass:: vtkm::filter::entity_extraction::Threshold
   :members:


Field Conversion
==============================

.. index::
   double: filter; field conversion

Field conversion modifies a field of a :class:`vtkm::cont::DataSet` to have roughly equivalent values but with a different structure.
These filters allow the field to be used in places where they otherwise would not be applicable.

Cell Average
------------------------------

.. index::
   double: cell average; filter

:class:`vtkm::filter::field_conversion::CellAverage` is the cell average filter.
It will take a data set with a collection of cells and a field defined on the points of the data set and create a new field defined on the cells.
The values of this new derived field are computed by averaging the values of the input field at all the incident points.
This is a simple way to convert a point field to a cell field.

.. doxygenclass:: vtkm::filter::field_conversion::CellAverage
   :members:

Point Average
------------------------------

.. index::
   double: point average; filter

:class:`vtkm::filter::field_conversion::PointAverage` is the point average filter.
It will take a data set with a collection of cells and a field defined on the cells of the data set and create a new field defined on the points.
The values of this new derived field are computed by averaging the values of the input field at all the incident cells.
This is a simple way to convert a cell field to a point field.

.. doxygenclass:: vtkm::filter::field_conversion::PointAverage
   :members:


Field Transform
==============================

.. index::
   double: filter; field transform

|VTKm| provides multiple filters to convert fields through some mathematical relationship.

Composite Vectors
------------------------------

.. index::
   double: filter; composite vectors

The :class:`vtkm::filter::field_transform::CompositeVectors` filter allows you to group multiple scalar fields into a single vector field.
This is convenient when importing data from a souce that stores vector components in separate arrays.

.. doxygenclass:: vtkm::filter::field_transform::CompositeVectors
   :members:

Cylindrical Coordinate System Transform
----------------------------------------

.. index::
   double: filter; cylindrical coordinate system transform
   single: coordinate system transform; cylindrical

The :class:`vtkm::filter::field_transform::CylindricalCoordinateTransform` filter is a coordinate system transformation.
The filter will take a data set and transform the points of the coordinate system.
By default, the filter will transform the coordinates from a Cartesian coordinate system to a cylindrical coordinate system.
The order for cylindrical coordinates is :math:`(R, \theta, Z)`.
The output coordinate system will be set to the new computed coordinates.

.. doxygenclass:: vtkm::filter::field_transform::CylindricalCoordinateTransform
   :members:

Field to Colors
------------------------------

.. index::
   double: filter; field to colors

The :class:`vtkm::filter::field_transform::FieldToColors` filter takes a field in a data set, looks up each value in a color table, and writes the resulting colors to a new field.
The color to be used for each field value is specified using a :class:`vtkm::cont::ColorTable` object.
:class:`vtkm::cont::ColorTable` objects are also used with |VTKm|'s rendering module and are described in :secref:`rendering:Color Tables`.

:class:`vtkm::filter::field_transform::FieldToColors` has three modes it can use to select how it should treat the input field.
These input modes are contained in :enum:`vtkm::filter::field_transform::FieldToColors::InputMode`.
Additionally, :class:`vtkm::filter::field_transform::FieldToColors` has different modes in which it can represent colors in its output.
These output modes are contained in :enum:`vtkm::filter::field_transform::FieldToColors::OutputMode`.

.. doxygenclass:: vtkm::filter::field_transform::FieldToColors
   :members:

Generate Ids
------------------------------

.. index::
   double: generate ids; filter

The :class:`vtkm::filter::field_transform::GenerateIds` filter creates point and/or cell fields that mimic the identifier for the respective element.

.. doxygenclass:: vtkm::filter::field_transform::GenerateIds
   :members:

Log Values
------------------------------

.. index::
   double: log; filter

The :class:`vtkm::filter::field_transform::LogValues` filter can be used to take the logarithm of all values in a field.
The filter is able to take the logarithm to a number of predefined bases identified by :enum:`vtkm::filter::field_transform::LogValues::LogBase`.

.. doxygenclass:: vtkm::filter::field_transform::LogValues
   :members:

Point Elevation
------------------------------

.. index::
   double: point elevation; filter
   double: elevation; filter

The :class:`vtkm::filter::field_transform::PointElevation` filter computes the "elevation" of a field of point coordinates in space.
:numref:`ex:PointElevation` gives a demonstration of the elevation filter.


.. doxygenclass:: vtkm::filter::field_transform::PointElevation
   :members:

Point Transform
------------------------------

.. index::
   double: point transform; filter
   double: transform; filter

The :class:`vtkm::filter::field_transform::PointTransform` filter performs affine transforms is the point transform filter.

.. doxygenclass:: vtkm::filter::field_transform::PointTransform
   :members:

Spherical Coordinate System Transform
----------------------------------------

.. index::
   double: filter; spherical coordinate system transform
   single: coordinate system transform; spherical

The :class:`vtkm::filter::field_transform::SphericalCoordinateTransform` filter is a coordinate system transformation.
The filter will take a data set and transform the points of the coordinate system.
By default, the filter will transform the coordinates from a Cartesian coordinate system to a spherical coordinate system.
The order for spherical coordinates is :math:`(R, \theta, \phi)` where :math:`R` is the radius, :math:`\theta` is the azimuthal angle and :math:`\phi` is the polar angle.
The output coordinate system will be set to the new computed coordinates.

.. doxygenclass:: vtkm::filter::field_transform::SphericalCoordinateTransform
   :members:

Warp
------------------------------

.. index::
   double: warp; filter

The :class:`vtkm::filter::field_transform::Warp` filter modifies points in a :class:`vtkm::cont::DataSet` by moving points along scaled direction vectors.
By default, the :class:`vtkm::filter::field_transform::Warp` filter modifies the coordinate system and writes its results to the coordiante system.
A vector field can be selected as directions, or a constant direction can be specified.
A constant direction is particularly useful for generating a carpet plot.
A scalar field can be selected to scale the displacement, and a constant scale factor adjustment can be specified.

.. doxygenclass:: vtkm::filter::field_transform::Warp
   :members:


Flow Analysis
==============================

.. index:: flow

Flow visualization is used to analyze vector fields that represent the movement of a fluid.
The basic operation of most flow visualization algorithms is particle advection, which traces the path a particle would take given the direction and speed dictated by the vector field.
There are multiple ways in which to represent flow in this manner, and consequently |VTKm| contains several filters that trace streams in different ways.
These filters inherit from :class:`vtkm::filter::flow::FilterParticleAdvection`, which provides several important methods.

.. doxygenclass:: vtkm::filter::flow::FilterParticleAdvection
   :members:

Flow filters operate either on a "steady state" flow that does not change or on an "unsteady state" flow that is continually changing over time.
An unsteady state filter must be executed multiple times for subsequent time steps.
The filter operates with data from two adjacent time steps.
This is managed by the :class:`vtkm::filter::flow::FilterParticleAdvectionUnsteadyState` superclass.

Streamlines
------------------------------

.. index::
   double: streamlines; filter
   single: flow; streamlines

*Streamlines* are a powerful technique for the visualization of flow fields.
A streamline is a curve that is parallel to the velocity vector of the flow field.
Individual streamlines are computed from an initial point location (seed) using a numerical
method to integrate the point through the flow field.

.. doxygenclass:: vtkm::filter::flow::Streamline
   :members:

The :class:`vtkm::filter::flow::Streamline` filter also uses several inherited methods: :func:`vtkm::filter::flow::FilterParticleAdvection::SetSeeds`, :func:`vtkm::filter::flow::FilterParticleAdvection::SetStepSize`, and :func:`vtkm::filter::flow::FilterParticleAdvection::SetNumberOfSteps`.

.. load-example:: Streamlines
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`vtkm::filter::flow::Streamline`.

Pathlines
------------------------------

.. index::
   double: pathlines; filter
   single: flow; pathlines

*Pathlines* are the analog to streamlines for time varying vector fields.
Individual pathlines are computed from an initial point location (seed) using a numerical method to integrate the point through the flow field.

This filter requires two data sets as input, which represent the data for two sequential time steps.
The "Previous" data set, which marks the data at the earlier time step, is passed into the filter throught the standard ``Execute`` method.
The "Next" data set, which marks the data at the later time step, is specified as state to the filter using methods.

.. doxygenclass:: vtkm::filter::flow::Pathline
   :members:

As an unsteady state flow filter, :class:`vtkm::filter::flow::Pathline` must be executed multiple times for subsequent time steps.
The filter operates with data from two adjacent time steps.
This is managed by the :class:`vtkm::filter::flow::FilterParticleAdvectionUnsteadyState` superclass.

The :class:`vtkm::filter::flow::Pathline` filter uses several other inherited methods: :func:`vtkm::filter::flow::FilterParticleAdvectionUnsteadyState::SetPreviousTime`, :func:`vtkm::filter::flow::FilterParticleAdvectionUnsteadyState::SetNextTime`, :func:`vtkm::filter::flow::FilterParticleAdvectionUnsteadyState::SetNextDataSet`, :func:`vtkm::filter::flow::FilterParticleAdvection::SetSeeds`, :func:`vtkm::filter::flow::FilterParticleAdvection::SetStepSize`, and :func:`vtkm::filter::flow::FilterParticleAdvection::SetNumberOfSteps`.

.. load-example:: Pathlines
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`vtkm::filter::flow::Pathline`.

Stream Surface
------------------------------

.. index::
   double: stream surface; filter
   single: flow; stream surface

A *stream surface* is defined as a continuous surface that is everywhere tangent to a specified vector field.
The :class:`vtkm::filter::flow::StreamSurface` filter computes a stream surface from a set of input points and the vector field of the input data set.
The stream surface is created by creating streamlines from each input point and then connecting adjacent streamlines with a series of triangles.

.. doxygenclass:: vtkm::filter::flow::StreamSurface
   :members:

.. load-example:: StreamSurface
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`vtkm::filter::flow::StreamSurface`.

Lagrangian Coherent Structures
------------------------------

.. index::
   double: FTLE; filter
   double: Lagrangian coherent structures; filter
   see: LCS; Lagrangian coherent structures
   see: finite time Lyapunov exponent; FTLE

Lagrangian coherent structures (LCS) are distinct structures present in a flow field that have a major influence over nearby trajectories over some interval of time.
Some of these structures may be sources, sinks, saddles, or vortices in the flow field.
Identifying Lagrangian coherent structures is part of advanced flow analysis and is an important part of studying flow fields.
These structures can be studied by calculating the finite time Lyapunov exponent (FTLE) for a flow field at various locations, usually over a regular grid encompassing the entire flow field.
If the provided input dataset is structured, then by default the points in this data set will be used as seeds for advection.
The :class:`vtkm::filter::flow::LagrangianStructures` filter is used to compute the FTLE of a flow field.

.. doxygenclass:: vtkm::filter::flow::LagrangianStructures
   :members:


Geometry Refinement
==============================

.. index:: geometry refinement

Geometry refinement modifies the geometry of a :class:`vtkm::cont::DataSet`.
It might add, change, or remove components of the structure, but the general representation will be the same.

Convert to a Point Cloud
------------------------------

.. index::
   double: convert to point cloud; filter
   single: meshless data

Data in a :class:`vtkm::cont::DataSet` is typically connected together by cells in a mesh structure.
However, it is sometimes the case where data are simply represented as a cloud of unconnected points.
These meshless data sets are best represented in a :class:`vtkm::cont::DataSet` by a collection of "vertex" cells.

The :class:`vtkm::filter::geometry_refinement::ConvertToPointCloud` filter converts a data to a point cloud.
It does this by throwing away any existing cell set and replacing it with a collection of vertex cells, one per point.
:class:`vtkm::filter::geometry_refinement::ConvertToPointCloud` is useful to add a cell set to a :class:`vtkm::cont::DataSet` that has points but no cells.
It is also useful to treat data as a collection of sample points rather than an interconnected mesh.

.. doxygenclass:: vtkm::filter::geometry_refinement::ConvertToPointCloud
   :members:

Shrink
------------------------------

.. index::
   double: shrink; filter
   single: exploded view

The :class:`vtkm::filter::geometry_refinement::Shrink` independently reduces the size of each class.
Rather than uniformly reduce the size of the whole data set (which can be done with :class:`vtkm::filter::field_transform::PointTransform`), this filter separates the cells from each other and shrinks them around their centroid.
This is useful for making an "exploded view" of the data where the facets of the data are moved away from each other to see inside.

.. doxygenclass:: vtkm::filter::geometry_refinement::Shrink
   :members:

Split Sharp Edges
------------------------------

.. index::
   double: split sharp edges; filter

The :class:`vtkm::filter::geometry_refinement::SplitSharpEdges` filter splits sharp manifold edges where the feature angle between the adjacent surfaces are larger than a threshold value.
This is most useful to preserve sharp edges when otherwise applying smooth shading during rendering.

.. doxygenclass:: vtkm::filter::geometry_refinement::SplitSharpEdges
   :members:

Tetrahedralize
------------------------------

.. index::
   double: tetrahedralize; filter

The :class:`vtkm::filter::geometry_refinement::Tetrahedralize` filter converts all the polyhedra in a :class:`vtkm::cont::DataSet` into tetrahedra.

.. doxygenclass:: vtkm::filter::geometry_refinement::Tetrahedralize
   :members:

Triangulate
------------------------------

.. index::
   double: triangulate; filter

The :class:`vtkm::filter::geometry_refinement::Triangulate` filter converts all the polyhedra in a :class:`vtkm::cont::DataSet` into tetrahedra.

.. doxygenclass:: vtkm::filter::geometry_refinement::Triangulate
   :members:

Tube
------------------------------

.. index::
   double: tube; filter

The :class:`vtkm::filter::geometry_refinement::Tube` filter generates a tube around each line and polyline in the input data set.

.. doxygenclass:: vtkm::filter::geometry_refinement::Tube
   :members:

.. load-example:: Tube
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`vtkm::filter::geometry_refinement::Tube`.

Vertex Clustering
------------------------------

.. index::
   double: vertex clustering; filter
   double: surface simplification; filter

The :class:`vtkm::filter::geometry_refinement::VertexClustering` filter simplifies a polygonal mesh.
It does so by dividing space into a uniform grid of bin and then merges together all points located in the same bin.
The smaller the dimensions of this binning grid, the fewer polygons will be in the output cells and the coarser the representation.
This surface simplification is an important operation to support :index:`level of detail` (:index:`LOD`) rendering in visualization applications.

.. doxygenclass:: vtkm::filter::geometry_refinement::VertexClustering
   :members:

.. load-example:: VertexClustering
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`vtkm::filter::geometry_refinement::VertexClustering`.


Mesh Information
==============================

.. index:: mesh information

|VTKm| provides several filters that derive information about the structure of the geometry.
This can be information about the shape of cells or their connections.

Cell Size Measurements
------------------------------

.. index::
   double: cell measures; filter

The :class:`vtkm::filter::mesh_info::CellMeasures` filter integrates the size of each cell in a mesh and reports the size in a new cell field.

.. doxygenclass:: vtkm::filter::mesh_info::CellMeasures
   :members:

By default, :class:`vtkm::filter::mesh_info::CellMeasures` will compute the measures of all types of cells.
It is sometimes desirable to limit the types of cells to measure to prevent the resulting field from mixing values of different units.
The appropriate measure to compute can be specified with the :enum:`vtkm::filter::mesh_info::IntegrationType` enumeration.

.. doxygenenum:: vtkm::filter::mesh_info::IntegrationType

Ghost Cell Classification
------------------------------

.. index::
   double: ghost cell classification; filter
   single: ghost cell; classify

The :class:`vtkm::filter::mesh_info::GhostCellClassify` filter determines which cells should be considered ghost cells in a structured data set.
The ghost cells are expected to be on the border.

.. todo:: Document ``vtkm::CellClassification``.

.. doxygenclass:: vtkm::filter::mesh_info::GhostCellClassify
   :members:

Mesh Quality Metrics
------------------------------

.. index::
   double: mesh quality; filter
   single: mesh information; quality

|VTKm| provides several filters to compute metrics about the mesh quality.
These filters produce a new cell field that holds a given metric for the shape of the cell.
The metrics for this filter come from the Verdict library, and
full mathematical descriptions for each metric can be found in the Verdict
documentation (Sandia technical report SAND2007-1751,
https://coreform.com/papers/verdict_quality_library.pdf).

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityArea
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityAspectGamma
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityAspectRatio
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityCondition
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityDiagonalRatio
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityDimension
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityJacobian
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityMaxAngle
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityMaxDiagonal
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityMinAngle
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityMinDiagonal
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityOddy
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityRelativeSizeSquared
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityScaledJacobian
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityShape
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityShapeAndSize
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityShear
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualitySkew
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityStretch
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityTaper
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityVolume
   :members:

.. doxygenclass:: vtkm::filter::mesh_info::MeshQualityWarpage
   :members:

The :class:`vtkm::filter::mesh_info::MeshQuality` filter consolidates all of these metrics into a single filter.
The metric to compute is selected with the :func:`vtkm::filter::mesh_info::MeshQuality::SetMetric()`.

.. doxygenclass:: vtkm::filter::mesh_info::MeshQuality
   :members:

The metric to compute is identified using the :enum:`vtkm::filter::mesh_info::CellMetric` enum.

.. doxygenenum:: vtkm::filter::mesh_info::CellMetric


Multi-Block
==============================

.. index:: multi-block

Data with multiple blocks are stored in :class:`vtkm::cont::PartitionedDataSet` objects.
Most |VTKm| filters operate correctly on :class:`vtkm::cont::PartitionedDataSet` just like they do with :class:`vtkm::cont::DataSet`.
However, there are some filters that are designed with operations specific to multi-block datasets.

AMR Arrays
------------------------------

.. index::
   double: AMR arrays; filter
   single: AMR; arrays

An AMR mesh is a :class:`vtkm::cont::PartitionedDataSet` with a special structure in the partitions.
Each partition has a :class:`vtkm::cont::CellSetStructured` cell set.
The partitions form a hierarchy of grids where each level of the hierarchy refines the one above.

:class:`vtkm::cont::PartitionedDataSet` does not explicitly store the structure of an AMR grid.
The :class:`vtkm::filter::multi_block::AmrArrays` filter determines the hierarchical structure of the AMR partitions and stores information about them in cell field arrays on each partition.

.. doxygenclass:: vtkm::filter::multi_block::AmrArrays
   :members:

.. didyouknow::
  The names of the generated field arrays arrays (e.g. ``vtkAmrLevel``) are chosen to be compatible with the equivalent arrays in VTK.
  This is why they use the prefix of "vtk" instead of "vtkm".
  Likewise, the flags used for ``vtkGhostType`` are compatible with VTK.

Merge Data Sets
------------------------------

.. index::
   double: merge data sets; filter

A :class:`vtkm::cont::PartitionedDataSet` can often be treated the same as a :class:`vtkm::cont::DataSet` as both can be passed to a filter's `Execute` method.
However, it is sometimes important to have all the data contained in a single ``DataSet``.
The :class:`vtkm::filter::multi_block::MergeDataSets` filter can do just that to the partitions of a `vtkm::cont::PartitionedDataSet`.

.. doxygenclass:: vtkm::filter::multi_block::MergeDataSets
   :members:


Resampling
==============================

All data in :class:`vtkm::cont::DataSet` objects are discrete representations.
It is sometimes necessary to resample this data in different ways.

Histogram Sampling
------------------------------

.. index::
   double: histogram sampling; filter

The :class:`vtkm::filter::resampling::HistSampling` filter randomly samples the points of an input data set.
The sampling is random but adaptive to preserve rare field value points.

.. doxygenclass:: vtkm::filter::resampling::HistSampling
   :members:

Probe
------------------------------

.. index::
   double: probe; filter

The :class:`vtkm::filter::resampling::Probe` filter maps the fields of one :class:`vtkm::cont::DataSet` onto another.
This is useful for redefining meshes as well as comparing field data from two data sets with different geometries.

.. doxygenclass:: vtkm::filter::resampling::Probe
   :members:


Vector Analysis
==============================

.. index::
   single: vector analysis

|VTKm|'s vector analysis filters compute operations on fields related to vectors (usually in 3-space).

Cross Product
------------------------------

.. index::
   double: cross product; filter

The :class:`vtkm::filter::vector_analysis::CrossProduct` filter computes the cross product of two vector fields for every element in the input data set.
The cross product filter computes (PrimaryField × SecondaryField).
The cross product computation works for either point or cell centered vector fields.

.. doxygenclass:: vtkm::filter::vector_analysis::CrossProduct
   :members:

Dot Product
------------------------------

.. index::
   double: dot product; filter

The :class:`vtkm::filter::vector_analysis::DotProduct` filter computes the dot product of two vector fields for every element in the input data set.
The dot product filter computes (PrimaryField . SecondaryField).
The dot product computation works for either point or cell centered vector fields.

.. doxygenclass:: vtkm::filter::vector_analysis::DotProduct
   :members:

Gradients
------------------------------

.. index::
   double: gradients; filter
   single: point gradients
   single: cell gradients

The :class:`vtkm::filter::vector_analysis::Gradient` filter estimates the gradient of a point based input field for every element in the input data set.
The gradient computation can either generate cell center based gradients, which are fast but less accurate, or more accurate but slower point based gradients.
The default for the filter is output as cell centered gradients, but can be changed by using the :func:`vtkm::filter::vector_analysis::Gradient::SetComputePointGradient` method.
The default name for the output fields is "Gradients", but that can be overridden as always using the :func:`vtkm::filter::vector_analysis::Gradient::SetOutputFieldName` method.

.. doxygenclass:: vtkm::filter::vector_analysis::Gradient
   :members:

Surface Normals
------------------------------

.. index::
   double: surface normals; filter
   single: normals

The :class:`vtkm::filter::vector_analysis::SurfaceNormals` filter computes the surface normals of a polygonal data set at its points and/or cells.
The filter takes a data set as input and by default, uses the active coordinate system to compute the normals.

.. doxygenclass:: vtkm::filter::vector_analysis::SurfaceNormals
   :members:

Vector Magnitude
------------------------------

.. index::
   double: vector magnitude; filter
   single: magnitude

The :class:`vtkm::filter::vector_analysis::VectorMagnitude` filter takes a field comprising vectors and computes the magnitude for each vector.
The vector field is selected as usual with the :func:`vtkm::filter::vector_analysis::VectorMagnitude::SetActiveField` method.
The default name for the output field is ``magnitude``, but that can be overridden as always using the :func:`vtkm::filter::vector_analysis::VectorMagnitude::SetOutputFieldName` method.

.. doxygenclass:: vtkm::filter::vector_analysis::VectorMagnitude
   :members:

ZFP Compression
==============================

.. index::
   double: zfp; filter
   single: filter; compression;
   single: compression; zfp

:class:`vtkm::filter::zfp::ZFPCompressor1D`, :class:`vtkm::filter::zfp::ZFPCompressor2D`, and :class:`vtkm::filter::zfp::ZFPCompressor3D` are a set of filters that take a 1D, 2D, and 3D field, respectively, and compresses the values using the compression algorithm ZFP.
       The field is selected as usual with the :func:`vtkm::filter::zfp::ZFPCompressor3D::SetActiveField()` method.
       The rate of compression is set using :func:`vtkm::filter::zfp::ZFPCompressor3D::SetRate()`.
       The default name for the output field is ``compressed``.

.. doxygenclass:: vtkm::filter::zfp::ZFPCompressor1D
   :members:

.. doxygenclass:: vtkm::filter::zfp::ZFPCompressor2D
   :members:

.. doxygenclass:: vtkm::filter::zfp::ZFPCompressor3D
   :members:

:class:`vtkm::filter::zfp::ZFPDecompressor1D`, :class:`vtkm::filter::zfp::ZFPDecompressor2D`, and :class:`vtkm::filter::zfp::ZFPDecompressor3D` are a set of filters that take a compressed 1D, 2D, and 3D field, respectively, and decompress the values using the compression algorithm ZFP.
       The field is selected as usual with the :func:`vtkm::filter::zfp::ZFPDecompressor3D::SetActiveField()` method.
       The rate of compression is set using :func:`vtkm::filter::zfp::ZFPDecompressor3D::SetRate()`.
       The default name for the output field is ``decompressed``.

.. doxygenclass:: vtkm::filter::zfp::ZFPDecompressor1D
   :members:

.. doxygenclass:: vtkm::filter::zfp::ZFPDecompressor2D
   :members:

.. doxygenclass:: vtkm::filter::zfp::ZFPDecompressor3D
   :members:
