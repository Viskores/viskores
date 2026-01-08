===================================
Generating Cell Sets
===================================

.. index::
   double: cell set; generate
   double: data set; generate

This chapter describes techniques for designing algorithms in |Viskores| that generate cell sets to be inserted in a :class:`viskores::cont::DataSet`.
Although :chapref:`dataset:Data Sets` on data sets describes how to create a data set, including defining its set of cells, these are serial functions run in the control environment that are not designed for computing geometric structures.
Rather, they are designed for specifying data sets built from existing data arrays, from inherently slow processes (such as file I/O), or for small test data.
In this chapter we discuss how to write worklets that create new mesh topologies by writing data that can be incorporated into a :class:`viskores::cont::CellSet`.

This chapter is constructed as a set of patterns that are commonly employed to build cell sets.
These techniques apply the worklet structures documented in :chapref:`worklet-types:Worklet Types`.
Although it is possible for these worklets to generate data of its own, the algorithms described here follow the more common use case of deriving one topology from another input data set.
This chapter is not (and cannot be) completely comprehensive by covering every possible mechanism for building cell sets.
Instead, we provide the basic and common patterns used in scientific visualization.


------------------------------
Single Cell Type
------------------------------

.. index::
   double: CellSetSingleType; generate

For our first example of algorithms that generate cell sets is one that creates a set of cells in which all the cells are of the same shape and have the same number of points.
Our motivating example is an algorithm that will extract all the edges from a cell set.
The resulting cell set will comprise a collection of line cells that represent the edges from the original cell set.
Since all cell edges can be represented as lines with two endpoints, we know all the output cells will be of the same type.
As we will see later in the example, we can use a :class:`viskores::cont::CellSetSingleType` to represent the cell connections.

It is rare that an algorithm generating a cell set will generate exactly one output cell for each input cell.
Thus, the first step in an algorithm generating a cell set is to count the number of cells each input item will create.
In our motivating example, this is the the number of edges for each input cell.

.. load-example:: GenerateMeshConstantShapeCount
   :file: GuideExampleGenerateMeshConstantShape.cxx
   :caption: A simple worklet to count the number of edges on each cell.

This count array generated in :numref:`ex:GenerateMeshConstantShapeCount` can be used in a :class:`viskores::worklet::ScatterCounting` of a subsequent worklet that generates the output cells.
(See :secref:`worklet-in-out:Scatter` for information on using a scatter with a worklet.)
We will see this momentarily.

.. didyouknow::
   If you happen to have an operation that you know will have the same count for every input cell, then you can skip the count step and use a :class:`viskores::worklet::ScatterUniform` instead of :class:`viskores::worklet::ScatterCounting`.
   Doing so will simplify the code and skip some computation.
   We cannot use :class:`viskores::worklet::ScatterUniform` in this example because different cell shapes have different numbers of edges and therefore different counts.
   However, if we were theoretically to make an optimization for 3D structured grids, we know that each cell is a hexahedron with 12 edges and could use a ``viskores::worklet::ScatterUniform<12>`` for that.

The second and final worklet we need to generate our wireframe cells is one that outputs the indices of an edge.
The worklet parenthesis' operator takes information about the input cell (shape and point indices) and an index of which edge to output.
The aforementioned :class:`viskores::worklet::ScatterCounting` provides a ``VisitIndex`` that signals which edge to output.
The worklet parenthesis operator returns the two indices for the line in, naturally enough, a :type:`viskores::Id2`.

.. load-example:: GenerateMeshConstantShapeGenIndices
   :file: GuideExampleGenerateMeshConstantShape.cxx
   :caption: A worklet to generate indices for line cells.

Our ultimate goal is to fill a :class:`viskores::cont::CellSetSingleType` object with the generated line cells.
A :class:`viskores::cont::CellSetSingleType` requires 4 items: the number of points, the constant cell shape, the constant number of points in each cell, and an array of connection indices.
The first 3 items are trivial.
The number of points can be taken from the input cell set as they are the same.
The cell shape and number of points are predetermined to be line and 2, respectively.
The last item, the array of connection indices, is what we are creating with the worklet in :numref:`ex:GenerateMeshConstantShapeGenIndices`.

However, there is a complication.
The connectivity array for :class:`viskores::cont::CellSetSingleType` is expected to be a flat array of :type:`viskores::Id` indices, not an array of :type:`viskores::Id2` objects.
We could jump through some hoops adjusting the :class:`viskores::worklet::ScatterCounting` to allow the worklet to output only one index of one cell rather than all indices of one cell.
But that would be overly complicated and inefficient.

A simpler approach is to use the :class:`viskores::cont::ArrayHandleGroupVec` fancy array handle, described in :secref:`fancy-array-handles:Grouped Vector Arrays`, to make a flat array of indices look like an array of ``Vec`` objects.
The following example shows what the :func:`viskores::filter::Filter::DoExecute` method in the associated filter would look like.
Note the use :func:`viskores::cont::make_ArrayHandleGroupVec` when calling :func:`viskores::filter::Filter::Invoke` on :exlineref:`ex:GenerateMeshConstantShapeInvoke:InvokeEdgeIndices` to make this conversion.

.. load-example:: GenerateMeshConstantShapeInvoke
   :file: GuideExampleGenerateMeshConstantShape.cxx
   :caption: Invoking worklets to extract edges from a cell set.

Another feature to note in :numref:`ex:GenerateMeshConstantShapeInvoke` is that the method calls :func:`viskores::worklet::ScatterCounting::GetOutputToInputMap` on the scatter object it creates and squirrels the map array away for later use (:exlineref:`ex:GenerateMeshConstantShapeInvoke:GetOutputToInputMap`).
The reason for this behavior is to implement mapping fields that are attached on the input cells to the indices of the output.
In practice, :func:`viskores::filter::Filter::DoExecute` is called on :class:`viskores::cont::DataSet` objects to create new :class:`viskores::cont::DataSet` objects.
The method in :numref:`ex:GenerateMeshConstantShapeInvoke` creates a new :class:`viskores::cont::CellSet`, but we also need a method to transform the fields on the data set.
The saved ``outputToInputCellMap`` array allows us to transform input fields to output fields.

The lambda function starting at :exlineref:`ex:GenerateMeshConstantShapeInvoke:FieldMapper` uses this saved ``outputToInputCellMap`` array and converts an array from an input cell field to an output cell field array.
It does this using the :func:`viskores::filter::MapFieldPermutation` helper function while using the ``outputToInputCellMap`` as the permutation array.


---------------------------------
Combining Like Elements
---------------------------------

Our motivating example in :secref:`generating-cell-sets:Single Cell Type` created a cell set with a line element representing each edge in some input data set.
However, on close inspection there is a problem with our algorithm: it is generating a lot of duplicate elements.
The cells in a typical mesh are connected to each other.
As such, they share edges with each other.
That is, the edge of one cell is likely to also be part of one or more other cells.
When multiple cells contain the same edge, the algorithm we created in :secref:`generating-cell-sets:Single Cell Type` will create multiple overlapping lines, one for each cell using the edge, as demonstrated in :numref:`fig:DuplicateEdges`.
What we really want is to have one line for every edge in the mesh rather than many overlapping lines.

.. figure:: ../../data/users-guide/images/DuplicateEdges.png
   :width: 100%
   :name: fig:DuplicateEdges

   Duplicate lines from extracted edges.
   Consider the small mesh at the left comprising a square and a triangle.
   If we count the edges in this mesh, we would expect to get 6.
   However, our naïve implementation in :secref:`generating-cell-sets:Single Cell Type` generates 7 because the shared edge (highlighted in red in the wireframe in the middle) is duplicated.
   As seen in the exploded view at right, one line is created for the square and one for the triangle.

In this section we will re-implement the algorithm to generate a wireframe by creating a line for each edge, but this time we will merge duplicate edges together.
Our first step is the same as before.
We need to count the number of edges in each input cell and use those counts to create a :class:`viskores::worklet::ScatterCounting` for subsequent worklets.
Counting the edges is a simple worklet.

.. load-example:: GenerateMeshCombineLikeCount
   :file: GuideExampleGenerateMeshCombineLike.cxx
   :caption: A simple worklet to count the number of edges on each cell.

.. index::
   double: worklet; reduce by key

In our previous version, we used the count to directly write out the lines.
However, before we do that, we want to identify all the unique edges and identify which cells share this edge.
This grouping is exactly the function that the reduce by key worklet type, described in :secref:`worklet-types:Reduce by Key`, is designed to accomplish.
The principal idea is to write a "key" that uniquely identifies the edge.
The reduce by key worklet can then group the edges by the key and allow you to combine the data for the edge.

Thus, our goal of finding duplicate edges hinges on producing a key where two keys are identical if and only if the edges are the same.
One straightforward key is to use the coordinates in 3D space by, say, computing the midpoint of the edge.
The main problem with using this point coordinates approach is that a computer can hold a point coordinate only with floating point numbers of limited precision.
Computer floating point computations are notorious for providing slightly different answers when the results should be the same.
For example, if an edge has endpoints at :math:`p_1` and :math:`p_2` and two different cells compute the midpoint as :math:`(p_1+p_2)/2` and :math:`(p_2+p_1)/2`, respectively, the answer is likely to be slightly different.
When this happens, the keys will not be the same and we will still produce 2 edges in the output.

Fortunately, there is a better choice for keys based on the observation that in the original cell set each edge is specified by endpoints that each have unique indices.
We can combine these 2 point indices to form a "canonical" descriptor of an edge (correcting for order).
(See Using indices to find common mesh elements is described by Miller et al. in "Finely-Threaded History-Based Topology Computation" in *Eurographics Symposium on Parallel Graphics and Visualization*, June 2014 for details on using indices to find common mesh elements.)
|Viskores| comes with a helper function, :func:`viskores::exec::CellEdgeCanonicalId`, defined in :file:`viskores/exec/CellEdge.h`, to produce these unique edge keys as :type:`viskores::Id2`'s.
Our second worklet produces these canonical edge identifiers.

.. load-example:: GenerateMeshCombineLikeGenIds
   :file: GuideExampleGenerateMeshCombineLike.cxx
   :caption: Worklet generating canonical edge identifiers.

Our third and final worklet generates the line cells by outputting the indices of each edge.
As hinted at earlier, this worklet is a reduce by key worklet, inheriting from :class:`viskores::worklet::WorkletReduceByKey`.
When the worklet is invoked, |Viskores| will collect the unique keys and call the worklet once for each unique edge.
Because there is no longer a consistent mapping from the generated lines to the elements of the input cell set, we need pairs of indices identifying the cells/edges from which the edge information comes.
We use these indices along with a connectivity structure produced by a ``WholeCellSetIn`` to find the information about the edge.
As shown later, these indices of cells and edges can be extracted from the :class:`viskores::worklet::ScatterCounting` used to execute the worklet back in :numref:`ex:GenerateMeshCombineLikeGenIds`.

As we did in :secref:`generating-cell-sets:Single Cell Type`, this worklet writes out the edge information in a :type:`viskores::Id2` (which in some following code will be created with an :class:`viskores::cont::ArrayHandleGroupVec`).

.. load-example:: GenerateMeshCombineLikeGenIndices
   :file: GuideExampleGenerateMeshCombineLike.cxx
   :caption: A worklet to generate indices for line cells from combined edges.

.. didyouknow::
   It so happens that the :type:`viskores::Id2`'s generated by :func:`viskores::exec::CellEdgeCanonicalId` contain the point indices of the two endpoints, which is enough information to create the edge.
   Thus, in this example it would be possible to forgo the steps of looking up indices through the cell set.
   That said, this is more often not the case, so for the purposes of this example we show how to construct cells without depending on the structure of the keys.

With these 3 worklets, it is now possible to generate all the information we need to fill a :class:`viskores::cont::CellSetSingleType` object.
A :class:`viskores::cont::CellSetSingleType` requires 4 items: the number of points, the constant cell shape, the constant number of points in each cell, and an array of connection indices.
The first 3 items are trivial.
The number of points can be taken from the input cell set as they are the same.
The cell shape and number of points are predetermined to be line and 2, respectively.

The last item, the array of connection indices, is what we are creating with the worklet in :numref:`ex:GenerateMeshCombineLikeGenIndices`.
The connectivity array for :class:`viskores::cont::CellSetSingleType` is expected to be a flat array of :type:`viskores::Id` indices, but the worklet needs to provide groups of indices for each cell (in this case as a :type:`viskores::Vec` object).
To reconcile what the worklet provides and what the connectivity array must look like, we use the :class:`viskores::cont::ArrayHandleGroupVec` fancy array handle, described in :secref:`fancy-array-handles:Grouped Vector Arrays`, to make a flat array of indices look like an array of :type:`viskores::Vec` objects.
The following example shows what the :func:`viskores::filter::Filter::DoExecute` method in the associated filter would look like.
Note the use of :func:`viskores::cont::make_ArrayHandleGroupVec` when calling :func:`viskores::filter::Filter::Invoke` at :exlineref:`ex:GenerateMeshCombineLikeInvoke:InvokeEdgeIndices` to make this conversion.

.. load-example:: GenerateMeshCombineLikeInvoke
   :file: GuideExampleGenerateMeshCombineLike.cxx
   :caption: Invoking worklets to extract unique edges from a cell set.

Another feature to note in :numref:`ex:GenerateMeshCombineLikeInvoke` is that because the cells returned in the output data are not the same as the input, the output cell fields must be similarly converted.
This is done by creating a lambda function in lines :exlineref:`{line}<ex:GenerateMeshCombineLikeInvoke:FieldMapperBegin>` -- :exlineref:`{line}<ex:GenerateMeshCombineLikeInvoke:FieldMapperEnd>` to convert the fields that is then passed to :func:`viskores::filter::Filter::CreateResult`, called at :exlineref:`ex:GenerateMeshCombineLikeInvoke:CreateResult`.
The mapping process reuses the object from before to extract the edges from the cells.
It first uses :func:`viskores::worklet::ScatterCounting::GetOutputToInputMap` on the scatter object it creates with a convenience function named :func:`viskores::filter::MapFieldPermutation` that duplicates the cell values for each edge.
It then uses the :class:`viskores::worklet::Keys` object from the duplicate edge removal with a convenience function named :func:`viskores::filter::MapFieldMergeAverage` that averages cell values for edges of adjacent cells.

.. didyouknow::
   For simplicity, :numref:`ex:GenerateMeshCombineLikeInvoke` is creating an intermediate array to hold the permutation.
   It would be possible to remove this temporary array for saved performance and memory, but this requires building a custom mapping function, which adds complexity.
   We will show an example of such a function in the following section.


-----------------------------------------------------
Faster Combining Like Elements with Hashes
-----------------------------------------------------

In the previous two sections we constructed worklets that took a cell set and created a new set of cells that represented the edges of the original cell set, which can provide a wireframe of the mesh.
In :secref:`generating-cell-sets:Single Cell Type` we provided a pair of worklets that generate one line per edge per cell.
In :secref:`generating-cell-sets:Combining Like Elements` we improved on this behavior by using a reduce by key worklet to find and merge shared edges.

If we were to time all the operations run in the later implementation to generate the wireframe such as the operations in :numref:`ex:GenerateMeshCombineLikeInvoke`, we would find that the vast majority of the time is not spent in the actual worklets.
Rather, the majority of the time is spent in collecting the like keys, which happens in the constructor of the :class:`viskores::worklet::Keys` object.
Internally, keys are collected by sorting them.
The most fruitful way to improve the performance of this algorithm is to improve the sorting behavior.

The details of how the sort works is dependent on the inner workings of the device adapter.
It turns out that the performance of the sort of the keys is highly dependent on the data type of the keys.
For example, sorting numbers stored in a 32-bit integer is often much faster than sorting groups of 2 or 3 64-bit integers.
This is particularly true when the sort is capable of performing a radix-based sort.

An easy way to convert collections of indices like those returned from :func:`viskores::exec::CellEdgeCanonicalId` to a 32-bit integer is to use a hash function.
To facilitate the creation of hash values, |Viskores| comes with a simple :func:`viskores::Hash` function (in the :file:`vtkm/Hash.h` header file).
:func:`viskores::Hash` takes a :type:`viskores::Vec` or |Veclike| object of integers and returns a value of type :type:`viskores::HashType` (an alias for a 32-bit integer).
This hash function uses the FNV-1a algorithm that is designed to create hash values that are quasi-random but deterministic.
This means that hash values of two different identifiers are unlikely to be the same.

.. doxygenfunction:: viskores::Hash

.. doxygentypedef:: viskores::HashType

That said, hash collisions can happen and become increasingly likely on larger data sets.
Therefore, if we wish to use hash values, we also have to add conditions that manage collisions when they happen.
Resolving hash value collisions adds overhead, but the time saved in faster sorting of hash values generally outweighs the overhead added by resolving collisions as studied by Lessley, et al. in "Techniques for Data-Parallel Searching for Duplicate Elements" in *IEEE Symposium on Large Data Analysis and Visualization*, October 2017.
In this section we will improve on the implementation given in :secref:`generating-cell-sets:Combining Like Elements` by using hash values for keys and resolving for collisions.

As always, our first step is to count the number of edges in each input cell.
These counts are used to create a :class:`viskores::worklet::ScatterCounting` for subsequent worklets.

.. load-example:: GenerateMeshHashCount
   :file: GuideExampleGenerateMeshHash.cxx
   :caption: A simple worklet to count the number of edges on each cell.

Our next step is to generate keys that can be used to find like elements.
As before, we will use the :func:`viskores::exec::CellEdgeCanonicalId` function to create a unique representation for each edge.
However, rather than directly use the value from :func:`viskores::exec::CellEdgeCanonicalId`, which is a :type:`viskores::Id2`, we will instead use that to generate a hash value.

.. load-example:: GenerateMeshHashGenHashes
   :file: GuideExampleGenerateMeshHash.cxx
   :caption: Worklet generating hash values.

The hash values generated by the worklet in :numref:`ex:GenerateMeshHashGenHashes` will be the same for two identical edges.
However, it is no longer guaranteed that two distinct edges will have different keys, and collisions of this nature become increasingly common for larger cell sets.
Thus, our next step is to resolve any such collisions.

The following example provides a worklet that goes through each group of edges associated with the same hash value (using a reduce by key worklet).
It identifies which edges are actually the same as which other edges, marks a local identifier for each unique edge group, and returns the number of unique edges associated with the hash value.

.. load-example:: GenerateMeshHashResolveCollisions
   :file: GuideExampleGenerateMeshHash.cxx
   :caption: Worklet to resolve hash collisions occurring on edge identifiers.

With all hash collisions correctly identified, we are ready to generate the connectivity array for the line elements.
This worklet uses a reduce by key worklet like the previous example, but this time we use a :class:`viskores::worklet::ScatterCounting` to run the worklet multiple times for hash values that contain multiple unique edges.
The worklet takes all the information it needs to reference back to the edges in the original mesh including a ``WholeCellSetIn``, look-back indices for the cells and respective edges, and the unique edge group indicators produced by :numref:`ex:GenerateMeshHashGenHashes`.

As in the previous sections, this worklet writes out the edge information in a :type:`viskores::Id2` (which in some following code will be created with an :class:`viskores::cont::ArrayHandleGroupVec`).

.. load-example:: GenerateMeshHashGenIndices
   :file: GuideExampleGenerateMeshHash.cxx
   :caption: A worklet to generate indices for line cells from combined edges and potential collisions.

With these 3 worklets, it is now possible to generate all the information we need to fill a :class:`viskores::cont::CellSetSingleType` object.
A :class:`viskores::cont::CellSetSingleType` requires 4 items: the number of points, the constant cell shape, the constant number of points in each cell, and an array of connection indices.
The first 3 items are trivial.
The number of points can be taken from the input cell set as they are the same.
The cell shape and number of points are predetermined to be line and 2, respectively.

The last item, the array of connection indices, is what we are creating with the worklet in :numref:`ex:GenerateMeshHashGenIndices`.
The connectivity array for :class:`viskores::cont::CellSetSingleType` is expected to be a flat array of :type:`viskores::Id` indices, but the worklet needs to provide groups of indices for each cell (in this case as a :type:`viskores::Vec` object).
To reconcile what the worklet provides and what the connectivity array must look like, we use the :class:`viskores::cont::ArrayHandleGroupVec` fancy array handle, described in :secref:`fancy-array-handles:Grouped Vector Arrays`, to make a flat array of indices look like an array of :type:`viskores::Vec` objects.

The following example shows what the :func:`viskores::filter::Filter::DoExecute` method in the associated filter would look like.

.. load-example:: GenerateMeshHashInvoke
   :file: GuideExampleGenerateMeshHash.cxx
   :caption: Invoking worklets to extract unique edges from a cell set using hash values.

As noted in :secref:`generating-cell-sets:Combining Like Elements`, in practice :func:`viskores::filter::Filter::DoExecute` is called on :class:`viskores::cont::DataSet` objects to create new :class:`viskores::cont::DataSet` objects.
Because :numref:`ex:GenerateMeshHashInvoke` creates a new :class:`viskores::cont::CellSet`, it also needs a mechanism to transform the fields on the data set.
To do this, we need to repurpose some of the data generated earlier in the algorithm.
This includes the ``outputToInputCellMap`` retrieved from the scatter object to replicate the cells for each edge, the ``cellToEdgeKeys`` :class:`viskores::worklet::Keys` object to find like hash values, the ``localEdgeIndices`` array used to identify edges in colliding hashes, and the ``hashCollisionScatter`` :class:`viskores::worklet::ScatterCounting` object used to separate edges from colliding hashes.

In :secref:`generating-cell-sets:Combining Like Elements` we used a convenience method to average a field attached to cells on the input to each unique edge in the output.
Unfortunately, that function does not take into account the collisions that can occur on the keys.
Instead we need a custom worklet to average those values that match the same unique edge.

.. load-example:: GenerateMeshHashAverageField
   :file: GuideExampleGenerateMeshHash.cxx
   :caption: A worklet and helper function to average values with the same key, resolving for collisions.

With this helper function, it is straightforward to process cell fields as demonstrated in :numref:`ex:GenerateMeshHashInvoke` lines :exlineref:`{line}<ex:GenerateMeshHashInvoke:FieldMapperBegin>` -- :exlineref:`{line}<ex:GenerateMeshHashInvoke:FieldMapperEnd>`.


--------------------------------------
Variable Cell Types
--------------------------------------

.. index:: 
   double: CellSetSingleExplicit; generate

So far in our previous examples we have demonstrated creating a cell set where every cell is the same shape and number of points (i.e. a :class:`viskores::cont::CellSetSingleType`).
However, it can also be the case where an algorithm must create cells of different types (into a :class:`viskores::cont::CellSetExplicit`).
The procedure for generating cells of different shapes is similar to that of creating a single shape.
There is, however, an added step of counting the size (in number of points) of each shape to build the appropriate structure for storing the cell connectivity.

Our motivating example is a filter that extracts all the unique faces in a cell set and stores them in a cell set of polygons.
This problem is similar to the one addressed in :secref:`generating-cell-sets:Single Cell Type`, :secref:`generating-cell-sets:Combining Like Elements`, and :secref:`generating-cell-sets:Faster Combining Like Elements with Hashes`.
In all cases it is necessary to find all subelements of each cell (in this case the faces instead of the edges).
It is also the case that we expect many faces to be shared among cells in the same way edges are shared among cells.
We will use the hash-based approach demonstrated in :secref:`generating-cell-sets:Faster Combining Like Elements with Hashes` except this time applied to faces instead of edges.

The main difference between the two extraction tasks is that whereas all edges are lines with two points, faces can come in different sizes.
A tetrahedron has triangular faces whereas a hexahedron has quadrilateral faces.
Pyramid and wedge cells have both triangular and quadrilateral faces.
Thus, in general the algorithm must be capable of outputting multiple cell types.

Our algorithm for extracting unique cell faces follows the same algorithm as that in :secref:`generating-cell-sets:Faster Combining Like Elements with Hashes`.
We first need three worklets (used in succession) to count the number of faces in each cell, to generate a hash value for each face, and to resolve hash collisions.
These are essentially the same as :numref:`ex:GenerateMeshHashCount`, :numref:`ex:GenerateMeshHashGenHashes`, and :numref:`ex:GenerateMeshHashResolveCollisions`, respectively, with superficial changes made such as changing ``Edge`` to ``Face``.
To make it simpler to follow the discussion, the code is not repeated here.

When extracting edges, these worklets provide everything necessary to write out line elements.
However, before we can write out polygons of different sizes, we first need to count the number of points in each polygon.
The following example does just that.
This worklet also writes out the identifier for the shape of the face, which we will eventually require to build a :class:`viskores::cont::CellSetExplicit`.
Also recall that we have to work with the information returned from the collision resolution to report on the appropriate unique cell face.

.. load-example:: GenerateMeshVariableShapeCountPointsInFace
   :file: GuideExampleGenerateMeshVariableShape.cxx
   :caption: A worklet to count the points in the final cells of extracted faces.

When extracting edges, we converted a flat array of connectivity information to an array of :type:`viskores::Vec`'s using an :class:`viskores::cont::ArrayHandleGroupVec`.
However, :class:`viskores::cont::ArrayHandleGroupVec` can only create :type:`viskores::Vec`'s of a constant size.
Instead, for this use case we need to use :class:`viskores::cont::ArrayHandleGroupVecVariable`.
As described in :secref:`fancy-array-handles:Grouped Vector Arrays`, :class:`viskores::cont::ArrayHandleGroupVecVariable` takes a flat array of values and an index array of offsets that points to the beginning of each group to represent as a |Veclike|.
The worklet in :numref:`ex:GenerateMeshVariableShapeCountPointsInFace` does not actually give us the array of offsets we need.
Rather, it gives us the count of each group.
We can get the offsets from the counts by using the :func:`viskores::cont::ConvertNumComponentsToOffsets` convenience function.

.. load-example:: GenerateMeshVariableShapeOffsetsArray
   :file: GuideExampleGenerateMeshVariableShape.cxx
   :caption: Converting counts of connectivity groups to offsets for :class:`viskores::cont::ArrayHandleGroupVecVariable`.

Once we have created an :class:`viskores::cont::ArrayHandleGroupVecVariable`, we can pass that to a worklet that produces the point connections for each output polygon.
The worklet is very similar to the one for creating edge lines, shown in :numref:`ex:GenerateMeshHashGenIndices`, but we have to correctly handle the |Veclike| of unknown type and size.

.. load-example:: GenerateMeshVariableShapeGenIndices
   :file: GuideExampleGenerateMeshVariableShape.cxx
   :caption: A worklet to generate indices for polygon cells of different sizes from combined edges and potential collisions.

With these worklets in place, we can implement a filter :func:`viskores::filter::Filter::DoExecute` as follows.

.. load-example:: GenerateMeshVariableShapeInvoke
   :file: GuideExampleGenerateMeshVariableShape.cxx
   :caption: Invoking worklets to extract unique faces from a cell set.

As noted previously, in practice :func:`viskores::filter::Filter::DoExecute` is called on :class:`viskores::cont::DataSet` objects to create new :class:`viskores::cont::DataSet` objects.
The process for doing so is no different from our previous algorithm as described at the end of :secref:`generating-cell-sets:Faster Combining Like Elements with Hashes` in :numref:`ex:GenerateMeshHashAverageField`.
