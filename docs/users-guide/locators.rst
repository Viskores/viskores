==============================
Locators
==============================

Locators are a special type of structure that allows you to take a point coordinate in space and then find a topological element that contains or is near that coordinate.
|VTKm| comes with multiple types of locators, which are categorized by the type of topological element that they find.
For example, a *cell locator* takes a coordinate in world space and finds the cell in a :class:`vtkm::cont::DataSet` that contains that cell.
Likewise, a *point locator* takes a coordinate in world space and finds a point from a :class:`vtkm::cont::CoordinateSystem` nearby.

Different locators differ in their interface slightly, but they all follow the same basic operation.
First, they are constructed and provided with one or more elements of a :class:`vtkm::cont::DataSet`.
Then they are built with a call to an :func:`vtkm::cont::CellLocatorBase::Update` method.
The locator can then be passed to a worklet as an ``ExecObject``, which will cause the worklet to get a special execution version of the locator that can do the queries.

.. didyouknow::
   Other visualization libraries, like |VTKm|'s big sister toolkit VTK, provide similar locator structures that allow iterative building by adding one element at a time.
   |VTKm| explicitly disallows this use case.
   Although iteratively adding elements to a locator is undoubtedly useful, such an operation will inevitably bottleneck a highly threaded algorithm in critical sections.
   This makes iterative additions to locators too costly to support in |VTKm|.


------------------------------
Cell Locators
------------------------------

.. index::
   double: locator; cell

Cell Locators in |VTKm| provide a means of building spatial search structures that can later be used to find a cell containing a certain point.
This could be useful in scenarios where the application demands the cell to which a point belongs to to achieve a certain functionality.
For example, while tracing a particle's path through a vector field, after every step we lookup which cell the particle has entered to interpolate the velocity at the new location to take the next step.

Using cell locators is a two step process.
The first step is to build the search structure.
This is done by instantiating one of the ``CellLocator`` classes, providing a cell set and coordinate system (usually from a :class:`vtkm::cont::DataSet`), and then updating the structure.
Once the cell locator is built, it can be used in the execution environment within a filter or worklet.

Building a Cell Locator
==============================

All cell locators in |VTKm| share the same basic interface for the required features of cell locators.
This generic interface provides methods to set the cell set (with :func:`vtkm::cont::CellLocatorBase::SetCellSet` and :func:`vtkm::cont::CellLocatorBase::GetCellSet`) and to set the coordinate system (with :func:`vtkm::cont::CellLocatorBase::SetCoordinates` and :func:`vtkm::cont::CellLocatorBase::GetCoordinates`).
Once the cell set and coordinates are provided, you may call :func:`vtkm::cont::CellLocatorBase::Update` to construct the search structures.
Although :func:`vtkm::cont::CellLocatorBase::Update` is called from the control environment, the search structure will be built on parallel devices.

.. load-example:: ConstructCellLocator
   :file: GuideExampleCellLocator.cxx
   :caption: Constructing a ``CellLocator``.

|VTKm| provides multiple implementations of cell locators.
All cell locator classes derive the abstract :class:`vtkm::cont::CellLocatorBase` class.

.. doxygenclass:: vtkm::cont::CellLocatorBase
   :members:

The choice of which cell locator to use depends on the structure of the cells and the regularity of the distribution.

Cell Locators for Structured Cell Sets
----------------------------------------

If your :class:`vtkm::cont::DataSet` has a cell set of type :class:`vtkm::cont::CellSetStructured`, this can give a locator information about the regular nature of the cells to more quickly identify cells.
The mechanism to find the cells then becomes dependent on the type of coordinates in the cell set.

.. index::
   double: uniform grid; locator

If the :class:`vtkm::cont::DataSet` contains a :class:`vtkm::cont::ArrayHandleUniformPointCoordinates` as the coordinate system, this is known as a uniform grid.
The cells are aligned with the world axes and have uniform spacing between them.
In this case, the :class:`vtkm::cont::CellLocatorUniformGrid` is highly optimized to find cells.

.. doxygenclass:: vtkm::cont::CellLocatorUniformGrid
   :members:

.. index::
   double: rectilinear grid; locator

In a related case, if the :class:`vtkm::cont::DataSet` with structured cells contains a :class:`vtkm::cont::ArrayHandleCartesianProduct` as the coordinate system, this is known as a rectilinear grid.
The cells are aligned with the world axes, but the spacing can vary between them.
In this case, the :class:`vtkm::cont::CellLocatorRectilinearGrid` is best to find cells.

.. doxygenclass:: vtkm::cont::CellLocatorRectilinearGrid
   :members:

For a :class:`vtkm::cont::DataSet` containing any other type of cell set or coordinate types, one of the locators for irregular cell sets described below must be used.

Cell Locators for Irregular Cell Sets
----------------------------------------

|VTKm| contains several locating strategies cells in irregular patterns in space.
These are typically used for cell sets with explicit connectivity or general positioning of points.
Although they will technically work on any type of data, they may be less efficient than those designed for a specific structure of data.

A good performing locator across many distributions of cells is :class:`vtkm::cont::CellLocatorTwoLevel`.
This search structure uses a single level of indirection to adapt to an uneven distribution of cells.
This tends to lead to a good balance between the number of ids to trace while finding cells, the number of cells that need to be checked, and the space to store the structure.

.. doxygenclass:: vtkm::cont::CellLocatorTwoLevel
   :members:

If you happen to know that the cells are evenly distributed across the bounds of the space, then the indirect reference of :class:`vtkm::cont::CellLocatorTwoLevel` is unnecessary.
Each bin in the grid will have approximately the same number of cells, and thus a single level can be used to remove some indirection in the lookup.
This is implemented with :class:`vtkm::cont::CellLocatorUniformBins`.

.. doxygenclass:: vtkm::cont::CellLocatorUniformBins
   :members:

In contrast, a very irregular data set may have multiple orders of magnitude difference in the size of its cells.
If the cell distribution is very irregular, the :class:`vtkm::cont::CellLocatorTwoLevel` can be left with bins containing a large number of cells in a regions with very small cells.
In these cases, :class:`vtkm::cont::CellLocatorBoundingIntervalHierarchy` can be used to capture the diversity in cell distribution.
:class:`vtkm::cont::CellLocatorBoundingIntervalHierarchy` builds a search structure by recursively dividing the space of cells.
This creates a deeper structure than :class:`vtkm::cont::CellLocatorTwoLevel`, so it can take longer to find a containing bin when searching for a cell.
However, the deeper structure means that each bin is guaranteed to contain a small number of cells.

.. doxygenclass:: vtkm::cont::CellLocatorBoundingIntervalHierarchy
   :members:

Cell Locators for Unknown Cell Sets
----------------------------------------

The previously described cell locators require you to know the type of cell set and coordinate system array to build a cell locator.
Often, this information is not available.
In these cases, |VTKm| provides a couple of classes to choose an appropriate locator.

If you are developing function that is templated on the type of cell set and coordinate system, you can use the :type:`vtkm::cont::CellLocatorChooser` templated type to automatically choose a locator of an appropriate type.

.. doxygentypedef:: vtkm::cont::CellLocatorChooser

.. load-example:: CellLocatorChooser
   :file: GuideExampleCellLocator.cxx
   :caption: Using :type:`vtkm::cont::CellLocatorChooser` to determine the cell locator type.

There are times when the type of cell locator cannot be easily determined at compile times.
In this case, the :class:`vtkm::cont::CellLocatorGeneral` can be used.
This locator will accept any type of cell set and coordinate system.
It will then choose at runtime the most appropriate cell locating structure to use.

.. doxygenclass:: vtkm::cont::CellLocatorGeneral
   :members:

.. load-example:: CellLocatorGeneral
   :file: GuideExampleCellLocator.cxx
   :caption: Using :class:`vtkm::cont::CellLocatorGeneral`.

Using Cell Locators in a Worklet
========================================

The :class:`vtkm::cont::CellLocatorBase` interface implements :class:`vtkm::cont::ExecutionObjectBase`.
This means that any ``CellLocator`` can be used in worklets as an ``ExecObject`` argument (as defined in the ``ControlSignature``).
See :chapref:`execution-objects:Execution Objects` for information on ``ExecObject`` arguments to worklets.

When a ``CellLocator`` class is passed as an ``ExecObject`` argument to a worklet :class:`vtkm::cont::Invoke`, the worklet receives a different object defined in the ``vtkm::exec`` namespace.
This ``CellLocator`` object provides a ``FindCell()`` method that identifies a containing cell given a point location in space.

.. commonerrors::
   Note that the ``CellLocator`` classes in the respective ``vtkm::cont`` and ``vtkm::exec`` namespaces are different objects with different interfaces despite the similar names.

Below is the documentation for :class:`vtkm::exec::CellLocatorUniformGrid`, which corresponds to the execution query struct provided by :class:`vtkm::cont::CellLocatorUniformGrid`.
That said, this interface is shared among all the execution query structs provided by all locator types.

.. doxygenclass:: vtkm::exec::CellLocatorUniformGrid
   :members:

The following example defines a simple worklet to get the value of a point field interpolated to a group of query point coordinates provided.

.. load-example:: UseCellLocator
   :file: GuideExampleCellLocator.cxx
   :caption: Using a ``CellLocator`` in a worklet.


------------------------------
Point Locators
------------------------------

.. index::
   double: locator; point

Point Locators in |VTKm| provide a means of building spatial search structures that can later be used to find the nearest neighbor a certain point.
This could be useful in scenarios where the closest pairs of points are needed.
For example, during halo finding of particles in cosmology simulations, pairs of nearest neighbors within certain linking length are used to form clusters of particles.

Using point locators is a two step process.
The first step is to build the search structure.
This is done by instantiating one of the ``PointLocator`` classes, providing a coordinate system (usually from a :class:`vtkm::cont::DataSet`) representing the location of points that can later be found through queries, and then updating the structure.
Once the point locator is built, it can be used in the execution environment within a filter or worklet.

Building a Point Locator
==============================

All point locators in |VTKm| share the same basic interface for the required features of point locators.
This generic interface provides methods to set the coordinate system (with :func:`vtkm::cont::PointLocatorBase::SetCoordinates` and :func:`vtkm::cont::PointLocatorBase::GetCoordinates`) of training points.
Once the coordinates are provided, you may call :func:`vtkm::cont::PointLocatorBase::Update` to construct the search structures.
Although :func:`vtkm::cont::PointLocatorBase::Update` is called from the control environment, the search structure will be built on parallel devices

.. load-example:: ConstructPointLocator
   :file: GuideExamplePointLocator.cxx
   :caption: Constructing a ``PointLocator``.

Point locators in |VTKm| derive the abstract :class:`vtkm::cont::PointLocatorBase` class.

.. doxygenclass:: vtkm::cont::PointLocatorBase
   :members:

|VTKm| implements a point locator named :class:`vtkm::cont::PointLocatorSparseGrid`.

.. doxygenclass:: vtkm::cont::PointLocatorSparseGrid
   :members:

Using Point Locators in a Worklet
========================================

The :class:`vtkm::cont::PointLocator::Base` interface implements :class:`vtkm::cont::ExecutionObjectBase`.
This means that any ``PointLocator`` can be used in worklets as an ``ExecObject`` argument (as defined in the ``ControlSignature``).
See :chapref:`execution-objects:Execution Objects` for information on ``ExecObject`` arguments to worklets.

When a ``PointLocator`` class is passed as an ``ExecObject`` argument to a worklet :class:`vtkm::cont::Invoke`, the worklet receives a different object defined in the ``vtkm::exec`` namespace.
This ``PointLocator`` object provides a ``FindNearestNeighbor`` method that identifies the nearest neighbor point given a point location in space.

.. commonerrors::
   Note that the ``PointLocator`` classes in the respective ``vtkm::cont`` and ``vtkm::exec`` namespaces are different objects with different interfaces despite the similar names.

Below is the documentation for :class:`vtkm::exec::PointLocatorSparseGrid`, which corresponds to the execution query struct provided by :class:`vtkm::cont::PointLocatorSparseGrid`.
That said, this interface is shared among all the execution query structs provided by all locator types.

.. doxygenclass:: vtkm::exec::PointLocatorSparseGrid
   :members:

The following example defines a simple worklet that finds points nearest to query locations.

.. load-example:: UsePointLocator
   :file: GuideExamplePointLocator.cxx
   :caption: Using a ``PointLocator`` in a worklet.
