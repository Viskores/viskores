==============================
Global Arrays and Topology
==============================

.. index:: worklet; creating

When writing an algorithm in |VTKm| by creating a worklet, the data each instance of the worklet has access to is intentionally limited.
This allows |VTKm| to provide safety from race conditions and other parallel programming difficulties.
However, there are times when the complexity of an algorithm requires all threads to have shared global access to a global structure.
This chapter describes worklet tags that can be used to pass data globally to all instances of a worklet.


------------------------------
Whole Arrays
------------------------------

.. index::
   single: whole array
   single: worklet; whole array
   single: control signature; whole array

A *whole array* argument to a worklet allows you to pass in a :class:`vtkm::cont::ArrayHandle`.
All instances of the worklet will have access to all the data in the :class:`vtkm::cont::ArrayHandle`.

.. commonerrors::
   The |VTKm| worklet invoking mechanism performs many safety checks to prevent race conditions across concurrently running worklets.
   Using a whole array within a worklet circumvents this guarantee of safety, so be careful when using whole arrays, especially when writing to whole arrays.

A whole array is declared by adding a :class:`WholeArrayIn`, a :class:`WholeArrayInOut`, or a :class:`WholeArrayOut` to the \controlsignature of a worklet.
The corresponding argument to the :class:`vtkm::cont::Invoker` should be an :class:`vtkm::cont::ArrayHandle`.
The :class:`vtkm::cont::ArrayHandle` must already be allocated in all cases, including when using :class:`WholeArrayOut`.
When the data are passed to the operator of the worklet, it is passed as an array portal object.
(Array portals are discussed in :secref:`basic-array-handles:Array Portals`.)
This means that the worklet can access any entry in the array with :func:`ArrayPortal::Get` and/or :func:`ArrayPortal::Set` methods.

We have already seen a demonstration of using a whole array in :numref:`ex:RandomArrayAccess` in :chapref:`worklet-types:Worklet Types` to perform a simple array copy.
Here we will construct a more thorough example of building functionality that requires random array access.

Let's say we want to measure the quality of triangles in a mesh.
A common method for doing this is using the equation

.. math::

   q = \frac{4a\sqrt{3}}{h_1^2 + h_2^2 + h_3^2}

where :math:`a` is the area of the triangle and :math:`h_1`, :math:`h_2`, and :math:`h_3` are the lengths of the sides.
We can easily compute this in a cell to point map, but what if we want to speed up the computations by reducing precision?
After all, we probably only care if the triangle is good, reasonable, or bad.
So instead, let's build a lookup table and then retrieve the triangle quality from that lookup table based on its sides.

The following example demonstrates creating such a table lookup in an array and using a worklet argument tagged with :class:`WholeArrayIn` to make it accessible.

.. load-example:: TriangleQualityWholeArray
   :file: GuideExampleTriangleQuality.cxx
   :caption: Using :class:`WholeArrayIn` to access a lookup table in a worklet.


------------------------------
Atomic Arrays
------------------------------

.. index::
   single: atomic array
   single: worklet; atomic array
   sintle: control signature; atomic array

One of the problems with writing to whole arrays is that it is difficult to coordinate the access to an array from multiple threads.
If multiple threads are going to write to a common index of an array, then you will probably need to use an *atomic array*.

An atomic array allows random access into an array of data, similar to a whole array.
However, the operations on the values in the atomic array allow you to perform an operation that modifies its value that is guaranteed complete without being interrupted and potentially corrupted.

.. commonerrors::
   Due to limitations in available atomic operations, atomic arrays can currently only contain :type:`vtkm::Int32` or :type:`vtkm::Int64` values.


To use an array as an atomic array, first add the :class:`AtomicArrayInOut` tag to the worklet's ``ControlSignature``.
The corresponding argument to the :class:`vtkm::cont::Invoker` should be an :class:`vtkm::cont::ArrayHandle`, which must already be allocated and initialized with values.

When the data are passed to the operator of the worklet, it is passed in a \vtkmexec{AtomicArrayExecutionObject} structure.

.. doxygenclass:: vtkm::exec::AtomicArrayExecutionObject
   :members:

.. commonerrors::
   Atomic arrays help resolve hazards in parallel algorithms, but they come at a cost.
   Atomic operations are more costly than non-thread-safe ones, and they can slow a parallel program immensely if used incorrectly.

.. index:: histogram

The following example uses an atomic array to count the bins in a histogram.
It does this by making the array of histogram bins an atomic array and then using an atomic add.
Note that this is not the fastest way to create a histogram.
We gave an implementation in :secref:`worklet-types:Reduce by Key` that is generally faster (unless your histogram happens to be very sparse).
|VTKm| also comes with a histogram worklet that uses a similar approach.

.. load-example:: SimpleHistogram
   :file: GuideExampleSimpleHistogram.cxx
   :caption: Using :class:`AtomicArrayInOut` to count histogram bins in a worklet.


------------------------------
Whole Cell Sets
------------------------------

.. index::
   single: whole cell set
   single: cell set; whole
   single: worklet; whole cell set
   single: control signature; whole cell set

:secref:`worklet-types:Topology Map` describes how to make a topology map filter that performs an operation on cell sets.
The worklet has access to a single cell element (such as point or cell) and its immediate connections.
But there are cases when you need more general queries on a topology.
For example, you might need more detailed information than the topology map gives or you might need to trace connections from one cell to the next.
To do this |VTKm| allows you to provide a *whole cell set* argument to a worklet that provides random access to the entire topology.

A whole cell set is declared by adding a :class:`WholeCellSetIn` to the worklet's ``ControlSignature``.
The corresponding argument to the :class:`vtkm::cont::Invoker` should be a :class:`vtkm::cont::CellSet` subclass or an :class:`vtkm::cont::UnknownCellSet` (both of which are described in :secref:`dataset:Cell Sets`).

The :class:`WholeCellSetIn` is templated and takes two arguments: the "visit" topology type and the "incident" topology type, respectively.
These template arguments must be one of the topology element tags, but for convenience you can use :class:`Point` and :class:`Cell` in lieu of :class:`vtkm::TopologyElementTagPoint` and :class:`vtkm::TopologyElementTagCell`, respectively.
The "visit" and "incident" topology types define which topological elements can be queried (visited) and which incident elements are returned.
The semantics of the "visit" and "incident" topology is the same as that for the general topology maps described in :secref:`worklet-types:Topology Map`.
You can look up an element of the "visit" topology by index and then get all of the "incident" elements from it.

For example, a ``WholeCellSetIn<Cell, Point>`` allows you to find all the points that are incident on each cell (as well as querying the cell shape). Likewise, a ``WholeCellSetIn<Point, Cell>`` allows you to find all the cells that are incident on each point.
The default parameters of :class:`WholeCellSetIn` are visiting cells with incident points.
That is, ``WholeCellSetIn<>`` is equivalent to ``WholeCellSetIn<Cell, Point>``.

When the cell set is passed to the operator of the worklet, it is passed in a special connectivity object.
The actual object type depends on the cell set, but :class:`vtkm::exec::ConnectivityExplicit` and are two common examples :class:`vtkm::exec::ConnectivityStructured`.

.. doxygenclass:: vtkm::exec::ConnectivityExplicit
   :members:
.. doxygenclass:: vtkm::exec::ConnectivityStructured
   :members:

All these connectivity objects share a common interface.
In particular, the share the types ``CellShapeTag`` and ``IndicesType``.
They also share the methods ``GetNumberOfElements()``, ``GetCellShape()``, ``GetNumberOfIndices()``, and ``GetIndices()``.

|VTKm| comes with several functions to work with the shape and index information returned from these connectivity objects.
Most of these methods are documented in :chapref:`working-with-cells:Working with Cells`.

Let us use the whole cell set feature to help us determine the "flatness" of a polygonal mesh.
We will do this by summing up all the angles incident on each on each point.
That is, for each point, we will find each incident polygon, then find the part of that polygon using the given point, then computing the angle at that point, and then summing for all such angles.
So, for example, in the mesh fragment shown in :numref:`fig:PointIncidentAngles` one of the angles attached to the middle point is labeled :math:`\theta_{j}`.

.. figure:: images/PointIncidentAngles.png
   :width: 25%
   :name: fig:PointIncidentAngles

   The angles incident around a point in a mesh.

We want a worklet to compute :math:`\sum_{j} \theta` for all such attached angles.
This measure is related (but not the same as) the curvature of the surface.
A flat surface will have a sum of :math:`2\pi`.
Convex and concave surfaces have a value less than :math:`2\pi`, and saddle surfaces have a value greater than :math:`2\pi`.

To do this, we create a visit points with cells worklet (:secref:`worklet-types:Visit Points with Cells`) that visits every point and gives the index of every incident cell.
The worklet then uses a whole cell set to inspect each incident cell to measure the attached angle and sum them together.

.. load-example:: SumOfAngles
   :file: GuideExampleSumOfAngles.cxx
   :caption: Using :class:`WholeCellSetIn` to sum the angles around each point.
