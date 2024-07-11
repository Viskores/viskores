==============================
Worklet Types
==============================

.. index::
   single: worklet
   single: worklet; creating

:chapref:`simple-worklets:Simple Worklets` introduces worklets and provides a simple example of creating a worklet to run an algorithm on a many core device.
Different operations in visualization can have different data access patterns, perform different execution flow, and require different provisions.
|VTKm| manages these different accesses, execution, and provisions by grouping visualization algorithms into common classes of operation and supporting each class with its own worklet type.

Each worklet type has a generic superclass that worklets of that particular type must inherit.
This makes the type of the worklet easy to identify.
The following list describes each worklet type provided by |VTKm| and the superclass that supports it.

.. index::
   double: worklet; field map

* **Field Map**
  A worklet deriving :class:`vtkm::worklet::WorkletMapField` performs a basic mapping operation that applies a function (the operator in the worklet) on all the field values at a single point or cell and creates a new field value at that same location.
  Although the intention is to operate on some variable over a mesh, a :class:`vtkm::worklet::WorkletMapField` may actually be applied to any array.
  Thus, a field map can be used as a basic :index:`map` operation.

.. index::
   double: worklet; topology map
   double: worklet; visit cells
   double: worklet; visit points

* **Topology Map**
  A worklet deriving :class:`vtkm::worklet::WorkletMapTopology` or one of its child classes performs a mapping operation that applies a function (the operator in the worklet) on all elements of a particular type (such as points or cells) and creates a new field for those elements.
  The basic operation is similar to a field map except that in addition to access fields being mapped on, the worklet operation also has access to incident fields.

  There are multiple convenience classes available for the most common types of topology mapping.
  :class:`vtkm::worklet::WorkletVisitCellsWithPoints` calls the worklet operation for each cell and makes every incident point available.
  This type of map also has access to cell structures and can interpolate point fields.
  Likewise, :class:`vtkm::worklet::WorkletVisitPointsWithCells` calls the worklet operation for each point and makes every incident cell available.

.. index::
   double: worklet; point neighborhood

* **Point Neighborhood**
  A worklet deriving from :class:`vtkm::worklet::WorkletPointNeighborhood` performs a mapping operation that applies a function (the operator in the worklet) on all points of a structured mesh.
  The basic operation is similar to a field map except that in addition to having access to the point being operated on, you can get the field values of nearby points within a neighborhood of a given size.
  Point neighborhood worklets can only applied to structured cell sets.

.. index::
   double: worklet; reduce by key

* **Reduce by Key**
  A worklet deriving :class:vtkm::worklet::WorkletReduceByKey` operates on an array of keys and one or more associated arrays of values.
  When a reduce by key worklet is invoked, all identical keys are collected and the worklet is called once for each unique key.
  Each worklet invocation is given a |Veclike| containing all values associated with the unique key.
  Reduce by key worklets are very useful for combining like items such as shared topology elements or coincident points.

The remainder of this chapter provides details on how to create worklets of each type.

.. todo:: Add link to new worklet types chapter when available (see below).


------------------------------
Field Map
------------------------------

.. index::
   double: worklet; field map
   single: map field

A worklet deriving :class:`vtkm::worklet::WorkletMapField` performs a basic mapping operation that applies a function (the operator in the worklet) on all the field values at a single point or cell and creates a new field value at that same location.
Although the intention is to operate on some variable over the mesh, a :class:`vtkm::worklet::WorkletMapField` can actually be applied to any array.

.. doxygenclass:: vtkm::worklet::WorkletMapField

A field map worklet supports the following tags in the parameters of its ``ControlSignature``.

.. doxygengroup:: WorkletMapFieldControlSigTags
   :content-only:

Furthermore, a field map worklet supports the following tags in the parameters of its ``ExecutionSignature``.

.. doxygengroup:: WorkletMapFieldExecutionSigTags
   :content-only:

Field maps most commonly perform basic calculator arithmetic, as demonstrated in the following example.

.. load-example:: UseWorkletMapField
   :file: GuideExampleUseWorkletMapField.cxx
   :caption: Implementation and use of a field map worklet.

Although simple, the :class:`vtkm::worklet::WorkletMapField` worklet type can be used (and abused) as a general parallel-for/scheduling mechanism.
In particular, the :class:`WorkIndex` execution signature tag can be used to get a unique index, the ``WholeArray*`` tags can be used to get random access to arrays, and the :class:`ExecObject` control signature tag can be used to pass execution objects directly to the worklet.
Whole arrays and execution objects are talked about in more detail in :chapref:`globals:Global Arrays and Topology` and Chapter \ref{chap:ExecutionObjects}, respectively, in more detail, but here is a simple example that uses the random access of :class:`WholeArrayOut` to make a worklet that copies an array in reverse order.

.. todo:: Fix reference to execution object chapter above.

.. load-example:: RandomArrayAccess
   :file: GuideExampleUseWorkletMapField.cxx
   :caption: Leveraging field maps and field maps for general processing.


------------------------------
Topology Map
------------------------------

A topology map performs a mapping that it applies a function (the operator in the worklet) on all the elements of a :class:`vtkm::cont::DataSet` of a particular type (i.e. point, edge, face, or cell).
While operating on the element, the worklet has access to data from all incident elements of another type.

There are several versions of topology maps that differ in what type of element being mapped from and what type of element being mapped to.
The subsequent sections describe these different variations of the topology maps.

Visit Cells with Points
==============================

.. index::
   double: worklet; visit cells

A worklet deriving :class:`vtkm::worklet::WorkletVisitCellsWithPoints` performs a mapping operation that applies a function (the operator in the worklet) on all the cells of a :class:`vtkm::cont::DataSet`.
While operating on the cell, the worklet has access to fields associated both with the cell and with all incident points.
Additionally, the worklet can get information about the structure of the cell and can perform operations like interpolation on it.

.. doxygenclass:: vtkm::worklet::WorkletVisitCellsWithPoints

A visit cells with points worklet supports the following tags in the parameters of its ``ControlSignature``.

.. doxygengroup:: WorkletVisitCellsWithPointsControlSigTags
   :content-only:

A visit cells with points worklet supports the following tags in the parameters of its ``ExecutionSignature``.

.. doxygengroup:: WorkletVisitCellsWithPointsExecutionSigTags
   :content-only:

Point to cell field maps are a powerful construct that allow you to interpolate point fields throughout the space of the data set.
See :chapref:`working-with-cells:Working with Cells` for a description on how to work with the cell information provided to the worklet.
The following example provides a simple demonstration that finds the geometric center of each cell by interpolating the point coordinates to the cell centers.

.. load-example:: UseWorkletVisitCellsWithPoints
   :file: GuideExampleUseWorkletVisitCellsWithPoints.cxx
   :caption: Implementation and use of a visit cells with points worklet.

Visit Points with Cells
==============================

.. index::
   double: worklet; visit points

A worklet deriving :class:`vtkm::worklet::WorkletVisitPointsWithCells` performs a mapping operation that applies a function (the operator in the worklet) on all the points of a :class:`vtkm::cont::DataSet`.
While operating on the point, the worklet has access to fields associated both with the point and with all incident cells.

.. doxygenclass:: vtkm::worklet::WorkletVisitPointsWithCells

A visit points with cells worklet supports the following tags in the parameters of its ``ControlSignature``.

.. doxygengroup:: WorkletVisitPointsWithCellsControlSigTags
   :content-only:

A visit points with cells worklet supports the following tags in the parameters of its ``ExecutionSignature``.

.. doxygengroup:: WorkletVisitPointsWithCellsExecutionSigTags
   :content-only:

Cell to point field maps are typically used for converting fields associated with cells to points so that they can be interpolated.
The following example does a simple averaging, but you can also implement other strategies such as a volume weighted average.

.. load-example:: UseWorkletVisitPointsWithCells
   :file: GuideExampleUseWorkletVisitPointsWithCells.cxx
   :caption: Implementation and use of a visit points with cells worklet.

..
   \subsection{General Topology Maps}
   \label{sec:WorkletMapTopology}

   \index{worklet types!topology map|(}
   \index{topology map worklet|(}
   \index{map topology|(}

   A worklet deriving :class:`vtkm::worklet::WorkletMapTopology` performs a mapping operation that applies a function (the operator in the worklet) on all the elements of a specified type from a :class:`vtkm::cont::DataSet`.
   While operating on each element, the worklet has access to fields associated both with that element and with all incident elements of a different specified type.

   The :class:`vtkm::worklet::WorkletMapTopology` class is a template with two template parameters.
   The first template parameter specifies the ``visit'' topology element, and the second parameter specifies the ``incident'' topology element.
   The worklet is scheduled such that each instance is associated with a particular ``visit'' topology element and has access to ``incident'' topology elements.

   \index{topology element tag|(}
   \index{tag!topology element|(}

   These visit and incident topology elements are specified with topology element tags, which are defined in the \vtkmheader{vtkm}{TopologyElementTag.h} header file.
   The available topology element tags are \vtkm{TopologyElementTagCell}, \vtkm{TopologyElementTagPoint}, \vtkm{TopologyElementTagEdge}, and \vtkm{TopologyElementTagFace}, which represent the cell, point, edge, and face elements, respectively.

   \index{topology element tag|)}
   \index{tag!topology element|)}

   :class:`vtkm::worklet::WorkletMapTopology` is a generic form of a topology map, and it can perform identically to the aforementioned forms of topology map with the correct template parameters.
   For example,
   \begin{quote}
     :class:`vtkm::worklet::WorkletMapTopology`\tparams{%
     \vtkm{TopologyElementTagCell}, %
     \vtkm{TopologyElementTagPoint}}
   \end{quote}
   is equivalent to the :class:`vtkm::worklet::WorkletVisitCellsWithPoints` class except the signature tags have different names.
   The names used in the specific topology map superclasses (such as :class:`vtkm::worklet::WorkletVisitCellsWithPoints`) tend to be easier to read and are thus preferable.
   However, the generic :class:`vtkm::worklet::WorkletMapTopology` is available for topology combinations without a specific superclass or to support more general mappings in a worklet.

   The general topology map worklet supports the following tags in the parameters of its ``ControlSignature``, which are equivalent to tags in the other topology maps but with different (more general) names.

   \begin{description}
   \item[\sigtag{CellSetIn}]
     This tag represents the cell set that defines the collection of elements the map will operate on.
     A \sigtag{CellSetIn} argument expects a \textidentifier{CellSet} subclass or an \textidentifier{UnknownCellSet} in the associated parameter of the \textidentifier{Invoker}.
     Each invocation of the worklet gets a cell shape tag.
     (Cell shapes and the operations you can do with cells are discussed in :chapref:`working-with-cells:Working with Cells`.)

     There must be exactly one \sigtag{CellSetIn} argument, and the worklet's \inputdomain must be set to this argument.

   \item[\sigtag{FieldInVisit}]
     This tag represents an input field that is associated with the ``visit'' element.
     A \sigtag{FieldInVisit} argument expects an \textidentifier{ArrayHandle} or an \textidentifier{UnknownArrayHandle} in the associated parameter of the \textidentifier{Invoker}.
     The size of the array must be exactly the number of cells.
     Each invocation of the worklet gets a single value out of this array.

   \item[\sigtag{FieldInIncident}]
     This tag represents an input field that is associated with the ``incident'' elements.
     A \sigtag{FieldInIncident} argument expects an \textidentifier{ArrayHandle} or an \textidentifier{UnknownArrayHandle} in the associated parameter of the \textidentifier{Invoker}.
     The size of the array must be exactly the number of ``incident'' elements.

     Each invocation of the worklet gets a |Veclike| object containing the field values for all the ``incident'' elements incident with the ``visit'' element being visited.
     If the field is a vector field, then the provided object is a \textidentifier{Vec} of \textidentifier{Vec}s.

   \item[\sigtag{FieldOut}]
     This tag represents an output field, which is necessarily associated with ``visit'' elements.
     A \sigtag{FieldOut} argument expects an \textidentifier{ArrayHandle} or an \textidentifier{UnknownArrayHandle} in the associated parameter of the \textidentifier{Invoker}.
     The array is resized before scheduling begins, and each invocation of the worklet sets a single value in the array.

   \item[\sigtag{FieldInOut}]
     This tag represents field that is both an input and an output, which is necessarily associated with ``visit'' elements.
     A \sigtag{FieldInOut} argument expects an \textidentifier{ArrayHandle} or an \textidentifier{UnknownArrayHandle} in the associated parameter of the \textidentifier{Invoker}.
     Each invocation of the worklet gets a single value out of this array, which is replaced by the resulting value after the worklet completes.

     \commoncontrolsignaturetags
   \end{description}

   A general topology map worklet supports the following tags in the parameters of its ``ExecutionSignature``.

   \begin{description}
     \numericexecutionsignaturetags

   \item[\sigtag{CellShape}]
     This tag produces a shape tag corresponding to the shape of the visited element.
     (Cell shapes and the operations you can do with cells are discussed in :chapref:`working-with-cells:Working with Cells`.)
     This is the same value that gets provided if you reference the \textsignature{CellSetIn} parameter.

     If the ``visit'' element is cells, the \sigtag{CellShape} clearly will match the shape of each cell.
     Other elements will have shapes to match their structures.
     Points have vertex shapes, edges have line shapes, and faces have some type of polygonal shape.

   \item[\sigtag{IncidentElementCount}]
     This tag produces a \vtkm{IdComponent} equal to the number of elements incident on the element being visited.
     The Vecs provided from a \textsignature{FieldInIncident} parameter will be the same size as \sigtag{IncidentElementCount}.

   \item[\sigtag{IncidentElementIndices}]
     This tag produces a |Veclike| object of \vtkm{Id}s giving the indices for all incident elements.
     The order of the entries is consistent with the values of all other \textsignature{FieldInIncident} arguments for the same worklet invocation.

     \commonexecutionsignaturetags
   \end{description}

   \index{map topology|)}
   \index{topology map worklet|)}
   \index{worklet types!topology map|)}


------------------------------
Neighborhood Mapping
------------------------------

.. index::
   double: worklet; neighborhood

|VTKm| provides a pair of worklets that allow easy access to data within a neighborhood of nearby elements.
This simplifies operations like smoothing a field by blending each value with that of its neighbors.
This can only be done on data sets with `vtkm::cont::CellSetStructured` cell sets where extended adjacencies are easy to find.
There are two flavors of the worklet: a point neighborhood worklet and a cell neighborhood worklet.

Point Neighborhood
==============================

.. index::
   double: worklet; point neighborhood

A worklet deriving :class:`vtkm::worklet::WorkletPointNeighborhood` performs a mapping operation that applies a function (the operator in the worklet) on all the points of a :class:`vtkm::cont::DataSet`.
While operating on the point, the worklet has access to field values on nearby points within a neighborhood.

.. doxygenclass:: vtkm::worklet::WorkletPointNeighborhood

A point neighborhood worklet supports the following tags in the parameters of its ``ControlSignature``.

.. doxygengroup:: WorkletPointNeighborhoodControlSigTags
   :content-only:

A point neighborhood worklet supports the following tags in the parameters of its ``ExecutionSignature``.

.. doxygengroup:: WorkletPointNeighborhoodExecutionSigTags
   :content-only:

Cell Neighborhood
==============================

.. index::
   double: worklet; cell neighborhood

A worklet deriving :class:`vtkm::worklet::WorkletCellNeighborhood` performs a mapping operation that applies a function (the operator in the worklet) on all the cells of a :class:`vtkm::cont::DataSet`.
While operating on the cell, the worklet has access to field values on nearby cells within a neighborhood.

.. doxygenclass:: vtkm::worklet::WorkletCellNeighborhood

A cell neighborhood worklet supports the following tags in the parameters of its ``ControlSignature``.

.. doxygengroup:: WorkletCellNeighborhoodControlSigTags
   :content-only:

A cell neighborhood worklet supports the following tags in the parameters of its ``ExecutionSignature``.

.. doxygengroup:: WorkletCellNeighborhoodExecutionSigTags
   :content-only:

Neighborhood Information
==============================

As stated earlier in this section, what makes a :class:`vtkm::worklet::WorkletPointNeighborhood` worklet special is its ability to get field information in a neighborhood surrounding a point rather than just the point itself.
This is done using the special ``FieldInNeighborhood`` in the ``ControlSignature``.
When you use this tag, rather than getting the single field value for the point, you get a :class:`vtkm::exec::FieldNeighborhood` object.

The :class:`vtkm::exec::FieldNeighborhood` class contains a :func:`vtkm::exec::FieldNeighborhood::Get` method that retrieves a field value relative to the local neighborhood.
:func:`vtkm::exec::FieldNeighborhood::Get` takes the :math:`i`, :math:`j`, :math:`k` index of the point with respect to the local point.
So, calling ``Get(0,0,0)`` retrieves at the point being visited.
Likewise, ``Get(-1,0,0)`` gets the value to the "left" of the point visited and ``Get(1,0,0)`` gets the value to the "right."

.. doxygenstruct:: vtkm::exec::FieldNeighborhood
   :members:

.. load-example:: GetNeighborhoodFieldValue
   :file: GuideExampleUseWorkletPointNeighborhood.cxx
   :caption: Retrieve neighborhood field value.

When performing operations on a neighborhood within the mesh, it is often important to know whether the expected neighborhood is contained completely within the mesh or whether the neighborhood extends beyond the borders of the mesh.
This can be queried using a :class:`vtkm::exec::BoundaryState` object, which is provided when a ``Boundary`` tag is listed in the ``ExecutionSignature``.

Generally, :class:`vtkm::exec::BoundaryState` allows you to specify the size of the neighborhood at runtime.
The neighborhood size is specified by a radius.
The radius specifies the number of items in each direction the neighborhood extends.
So, for example, a point neighborhood with radius 1 would contain a :math:`3\times3\times3` neighborhood centered around the point.
Likewise, a point neighborhood with radius 2 would contain a :math:`5\times5\times5` neighborhood centered around the point.
:class:`vtkm::exec::BoundaryState` provides several methods to determine if the neighborhood is contained in the mesh.

.. doxygenstruct:: vtkm::exec::BoundaryState
   :members:

The :func:`vtkm::exec::BoundaryState::MinNeighborIndices` and :func:`vtkm::exec::BoundaryState::MaxNeighborIndices` are particularly useful for iterating over the valid portion of the neighborhood.

.. load-example:: GetNeighborhoodBoundary
   :file: GuideExampleUseWorkletPointNeighborhood.cxx
   :caption: Iterating over the valid portion of a neighborhood.

Convolving Small Kernels
==============================

A common use case for point neighborhood worklets is to convolve a small kernel with a structured mesh.
A very simple example of this is averaging out the values the values within some distance to the central point.
This has the effect of smoothing out the field (although smoothing filters with better properties exist).
The following example shows a worklet that applies this simple "box" averaging.

.. load-example:: UseWorkletPointNeighborhood
   :file: GuideExampleUseWorkletPointNeighborhood.cxx
   :caption: Implementation and use of a point neighborhood worklet.


------------------------------
Reduce by Key
------------------------------

.. index::
   double: worklet; reduce by key

A worklet deriving :class:`vtkm::worklet::WorkletReduceByKey` operates on an array of keys and one or more associated arrays of values.
When a reduce by key worklet is invoked, all identical keys are collected and the worklet is called once for each unique key.
Each worklet invocation is given a |Veclike| containing all values associated with the unique key.
Reduce by key worklets are very useful for combining like items such as shared topology elements or coincident points.

.. figure:: images/ReduceByKeys.png
   :width: 4in
   :name: fig:ReduceByKey

   The collection of values for a reduce by key worklet.

:numref:`fig:ReduceByKey` shows a pictorial representation of how |VTKm| collects data for a reduce by key worklet.
All calls to a reduce by key worklet has exactly one array of keys.
The key array in this example has 4 unique keys: 0, 1, 2, 4.
These 4 unique keys will result in 4 calls to the worklet function.
This example also has 2 arrays of values associated with the keys.
(A reduce by keys worklet can have any number of values arrays.)

When the worklet is invoked, all these common keys will be collected with their associated values.
The parenthesis operator of the worklet will be called once per each unique key.
The worklet call will be given a |Veclike| containing all values that have the key.

``WorkletReduceByKey`` Reference
===================================

.. doxygenclass:: vtkm::worklet::WorkletReduceByKey

A reduce by key worklet supports the following tags in the parameters of its ``ControlSignature``.

.. doxygengroup:: WorkletReduceByKeyControlSigTags
   :content-only:

A reduce by key worklet supports the following tags in the parameters of its ``ExecutionSignature``.

.. doxygengroup:: WorkletReduceByKeyExecutionSigTags
   :content-only:

Key Objects
==============================

As specified in its documentation, the ``InputDomain`` of a ``WorkletReducedByKey`` has to be a ``KeysIn`` argument.
Unlike simple mapping worklets, the control environment object passed as the ``KeysIn`` cannot be a simple :class:`vtkm::cont::ArrayHandle`.
Rather, this argument has to be given a :class:`vtkm::worklet::Keys` object.
This object manages an array of keys by reorganizing (i.e. sorting) the keys and finding duplicated keys that should be merged.
A :class:`vtkm::worklet::Keys` object can be constructed by simply providing a :class:`vtkm::cont::ArrayHandle` to use as the keys.

.. doxygenclass:: vtkm::worklet::Keys
   :members:

Reduce by Key Examples
==============================

As stated earlier, the reduce by key worklet is useful for collecting like values.
To demonstrate the reduce by key worklet, we will create a simple mechanism to generate a :index:`histogram` in parallel.
(|VTKm| comes with its own histogram implementation, but we create our own version here for a simple example.)
The way we can use the reduce by key worklet to compute a histogram is to first identify which bin of the histogram each value is in, and then use the bin identifiers as the keys to collect the information.
To help with this example, we will first create a helper class named ``BinScalars`` that helps us manage the bins.

.. load-example:: BinScalars
   :file: GuideExampleUseWorkletReduceByKey.cxx
   :caption: A helper class to manage histogram bins.

Using this helper class, we can easily create a simple map worklet that takes values, identifies a bin, and writes that result out to an array that can be used as keys.

.. load-example:: IdentifyBins
   :file: GuideExampleUseWorkletReduceByKey.cxx
   :caption: A simple map worklet to identify histogram bins, which will be used as keys.

Once you generate an array to be used as keys, you need to make a :class:`vtkm::worklet::Keys` object.
The :class:`vtkm::worklet::Keys` object is what will be passed to the :class:`vtkm::cont::Invoker` for the argument associated with the ``KeysIn`` ``ControlSignature`` tag.
This of course happens in the control environment after calling the :class:`vtkm::cont::Invoker` for our worklet for generating the keys.

.. load-example:: CreateKeysObject
   :file: GuideExampleUseWorkletReduceByKey.cxx
   :caption: Creating a :class:`vtkm::worklet::Keys` object.

Now that we have our keys, we are finally ready for our reduce by key worklet.
A histogram is simply a count of the number of elements in a bin.
In this case, we do not really need any values for the keys.
We just need the size of the bin, which can be identified with the internally calculated ``ValueCount``.

A complication we run into with this histogram filter is that it is possible for a bin to be empty.
If a bin is empty, there will be no key associated with that bin, and the :class:`vtkm::cont::Invoker` will not call the worklet for that bin/key.
To manage this case, we have to initialize an array with 0's and then fill in the non-zero entities with our reduce by key worklet.
We can find the appropriate entry into the array by using the key, which is actually the bin identifier, which doubles as an index into the histogram.
The following example gives the implementation for the reduce by key worklet that fills in positive values of the histogram.

.. load-example:: CountBins
   :file: GuideExampleUseWorkletReduceByKey.cxx
   :caption: A reduce by key worklet to write histogram bin counts.

The previous example demonstrates the basic usage of the reduce by key worklet to count common keys.
A more common use case is to collect values associated with those keys, do an operation on those values, and provide a "reduced" value for each unique key.
The following example demonstrates such an operation by providing a worklet that finds the average of all values in a particular bin rather than counting them.

.. load-example:: AverageBins
   :file: GuideExampleUseWorkletReduceByKey.cxx
   :caption: A worklet that averages all values with a common key.

To complete the code required to average all values that fall into the same bin, the following example shows the full code required to invoke such a worklet.
Note that this example repeats much of the previous examples, but shows it in a more complete context.

.. load-example:: CombineSimilarValues
   :file: GuideExampleUseWorkletReduceByKey.cxx
   :caption: Using a reduce by key worklet to average values falling into the same bin.
