===================================
Extended Filter Implementations
===================================

.. index::
   double: filter; implementation

In :chapref:`simple-worklets:Simple Worklets` and :chapref:`worklet-types:Worklet Types` we discuss how to implement an algorithm in the |VTKm| framework by creating a worklet.
For simplicity, worklet algorithms are wrapped in what are called filter objects for general usage.
:chapref:`running-filters:Running Filters` introduces the concept of filters, and :chapref:`provided-filters:Provided Filters` documents those that come with the |VTKm| library.
:chapref:`basic-filter-impl:Basic Filter Implementation` gives a brief introduction on implementing filters.
This chapter elaborates on building new filter objects by introducing new filter types.
These will be used to wrap filters around the extended worklet examples in :chapref:`worklet-types:Worklet Types`.

Unsurprisingly, the base filter objects are contained in the ``vtkm::filter`` package.
In particular, all filter objects inherit from :class:`vtkm::filter::Filter`, either directly or indirectly.
The filter implementation must override the protected pure virtual method :func:`vtkm::filter::Filter::DoExecute`.
The base class will call this method to run the operation of the filter.

The :func:`vtkm::filter::Filter::DoExecute` method has a single argument that is a :class:`vtkm::cont::DataSet`.
The :class:`vtkm::cont::DataSet` contains the data on which the filter will operate.
:func:`vtkm::filter::Filter::DoExecute` must then return a new :class:`vtkm::cont::DataSet` containing the derived data.
The :class:`vtkm::cont::DataSet` should be created with one of the :func:`vtkm::filter::Filter::CreateResult` methods.

A filter implementation may also optionally override the :func:`vtkm::filter::Filter::DoExecutePartitions`.
This method is similar to :func:`vtkm::filter::Filter::DoExecute` except that it takes and returns a :class:`vtkm::cont::PartitionedDataSet` object.
If a filter does not provide a :func:`vtkm::filter::Filter::DoExecutePartitions` method, then if given a :class:`vtkm::cont::PartitionedDataSet`, the base class will call :func:`vtkm::filter::Filter::DoExecute` on each of the partitions and build a :class:`vtkm::cont::PartitionedDataSet` with the results.

In addition to (or instead of) operating on the geometric structure of a :class:`vtkm::cont::DataSet`, a filter will commonly take one or more fields from the input :class:`vtkm::cont::DataSet` and write one or more fields to the result.
For this reason, :class:`vtkm::filter::Filter` provides convenience methods to select input fields and output field names.

It also provides a method named :func:`vtkm::filter::Filter::GetFieldFromDataSet` that can be used to get the input fields from the :class:`vtkm::cont::DataSet` passed to :func:`vtkm::filter::Filter::DoExecute`.
When getting a field with :func:`vtkm::filter::Filter::GetFieldFromDataSet`, you get a :class:`vtkm::cont::Field` object.
Before you can operate on the :class:`vtkm::cont::Field`, you have to convert it to a :class:`vtkm::cont::ArrayHandle`.
:func:`vtkm::filter::Filter::CastAndCallScalarField` can be used to do this conversion.
It takes the field object as the first argument and attempts to convert it to an :class:`vtkm::cont::ArrayHandle` of different types.
When it finds the correct type, it calls the provided functor with the appropriate :class:`vtkm::cont::ArrayHandle`.
The similar :func:`vtkm::filter::Filter::CastAndCallVecField` does the same thing to find an :class:`vtkm::cont::ArrayHandle` with :class:`vtkm::Vec`'s of a selected length, and :func:`vtkm::filter::Filter::CastAndCallVariableVecField` does the same thing but will find :class:`vtkm::Vec`'s of any length.

The remainder of this chapter will provide some common patterns of filter operation based on the data they use and generate.


-----------------------------------
Deriving Fields from other Fields
-----------------------------------

.. index::
   double: filter; field

A common type of filter is one that generates a new field that is derived from one or more existing fields or point coordinates on the data set.
For example, mass, volume, and density are interrelated, and any one can be derived from the other two.
Typically, you would use :func:`vtkm::filter::Filter::GetFieldFromDataSet` to retrieve the input fields, one of the :func:`vtkm::filter::Filter::CastAndCall` methods to resolve the array type of the field, and finally use :func:`vtkm::filter::Filter::CreateResultField` to produce the output.

In this section we provide an example implementation of a field filter that wraps the "magnitude" worklet provided in :numref:`ex:UseWorkletMapField`.
By C++ convention, object implementations are split into two files.
The first file is a standard header file with a :file:`.h` extension that contains the declaration of the filter class without the implementation.
So we would expect the following code to be in a file named :file:`FieldMagnitude.h`.

.. load-example:: UseFilterField
   :file: GuideExampleUseWorkletMapField.cxx
   :caption: Header declaration for a field filter.

.. index::
   double: export macro; filter

You may notice in :exlineref:`UseFilterField:Export` there is a special macro names :c:macro:`VTKM_FILTER_VECTOR_CALCULUS_EXPORT`.
This macro tells the C++ compiler that the class ``FieldMagnitude`` is going to be exported from a library.
More specifically, the CMake for |VTKm|'s build will generate a header file containing this export macro for the associated library.
By |VTKm|'s convention, a filter in the ``vtkm::filter::vector_calculus`` will be defined in the :file:`vtkm/filter/vector_calculus` directory.
When defining the targets for this library, CMake will create a header file named :file:`vtkm_filter_vector_calculus.h` that contains the macro named :c:macro:`VTKM_FILTER_VECTOR_CALCULUS_EXPORT`.
This macro will provide the correct modifiers for the particular C++ compiler being used to export the class from the library.
If this macro is left out, then the library will work on some platforms, but on other platforms will produce a linker error for missing symbols.

Once the filter class is declared in the :file:`.h` file, the implementation filter is by convention given in a separate :file:`.cxx` file.
So the continuation of our example that follows would be expected in a file named :file:`FieldMagnitude.cxx`.

.. load-example:: FilterFieldImpl
   :file: GuideExampleUseWorkletMapField.cxx
   :caption: Implementation of a field filter.

The implementation of :func:`vtkm::filter::Filter::DoExecute` first pulls the input field from the provided :class:`vtkm::cont::DataSet` using :func:`vtkm::filter::Filter::GetFieldFromDataSet`.
It then uses :func:`vtkm::filter::Filter::CastAndCallVecField` to determine what type of :class:`vtkm::cont::ArrayHandle` is contained in the input field.
That calls a lambda function that invokes a worklet to create the output field.

.. doxygenfunction:: vtkm::filter::Filter::CastAndCallVecField(const vtkm::cont::UnknownArrayHandle&, Functor&&, Args&&...) const
.. doxygenfunction:: vtkm::filter::Filter::CastAndCallVecField(const vtkm::cont::Field&, Functor&&, Args&&...) const

.. didyouknow::
   The filter implemented in :numref:`ex:FilterFieldImpl` is limited to only find the magnitude of :class:`vtkm::Vec`'s with 3 components.
   It may be the case you wish to implement a filter that operates on :class:`vtkm::Vec`'s of multiple sizes (or perhaps even any size).
   :chapref:`unknown-array-handle:Unknown Array Handles` discusses how you can use the :class:`vtkm::cont::UnknownArrayHandle` contained in the :class:`vtkm::cont::Field` to more expressively decide what types to check for.

.. doxygenfunction:: vtkm::filter::Filter::CastAndCallVariableVecField(const vtkm::cont::UnknownArrayHandle&, Functor&&, Args&&...) const
.. doxygenfunction:: vtkm::filter::Filter::CastAndCallVariableVecField(const vtkm::cont::Field&, Functor&&, Args&&...) const

.. todo:: Fix reference to unknown array handle above.

Finally, :func:`vtkm::filter::Filter::CreateResultField` generates the output of the filter.
Note that all fields need a unique name, which is the reason for the second argument to :func:`vtkm::filter::Filter::CreateResult`.
The :class:`vtkm::filter::Filter` base class contains a pair of methods named :func:`vtkm::filter::Filter::SetOutputFieldName` and :func:`vtkm::filter::Filter::GetOutputFieldName` to allow users to specify the name of output fields.
The :func:`vtkm::filter::Filter::DoExecute` method should respect the given output field name.
However, it is also good practice for the filter to have a default name if none is given.
This might be simply specifying a name in the constructor, but it is worthwhile for many filters to derive a name based on the name of the input field.


------------------------------
Deriving Fields from Topology
------------------------------

.. index::
   double: filter; using cells

The previous example performed a simple operation on each element of a field independently.
However, it is also common for a "field" filter to take into account the topology of a data set.
In this case, the implementation involves pulling a :class:`vtkm::cont::CellSet` from the input :class:`vtkm::cont::DataSet` and performing operations on fields associated with different topological elements.
The steps involve calling :func:`vtkm::cont::DataSet::GetCellSet` to get access to the :class:`vtkm::cont::CellSet` object and then using topology-based worklets, described in :secref:`worklet-types:Topology Map`, to operate on them.

In this section we provide an example implementation of a field filter on cells that wraps the "cell center" worklet provided in :numref:`ex:UseWorkletVisitCellsWithPoints`.

.. load-example:: UseFilterFieldWithCells
   :file: GuideExampleUseWorkletVisitCellsWithPoints.cxx
   :caption: Header declaration for a field filter using cell topology.

As with any subclass of :class:`vtkm::filter::Filter`, the filter implements :func:`vtkm::filter::Filter::DoExecute`, which in this case invokes a worklet to compute a new field array and then return a newly constructed :class:`vtkm::cont::DataSet` object.

.. load-example:: FilterFieldWithCellsImpl
   :file: GuideExampleUseWorkletVisitCellsWithPoints.cxx
   :caption: Implementation of a field filter using cell topology.

.. todo:: The CastAndCall is too complex here. Probably should add a CastAndCallScalarOrVec to FilterField.


------------------------------
Data Set Filters
------------------------------

.. index::
   double: filter; data set

Sometimes, a filter will generate a data set with a new cell set based off the cells of an input data set.
For example, a data set can be significantly altered by adding, removing, or replacing cells.

As with any filter, data set filters can be implemented in classes that derive the :class:`vtkm::filter::Filter` base class and implement its :func:`vtkm::filter::Filter::DoExecute` method.

In this section we provide an example implementation of a data set filter that wraps the functionality of extracting the edges from a data set as line elements.
Many variations of implementing this functionality are given in Chapter~\ref{chap:GeneratingCellSets}.
Suffice it to say that a pair of worklets will be used to create a new :class:`vtkm::cont::CellSet`, and this :class:`vtkm::cont::CellSet` will be used to create the result :class:`vtkm::cont::DataSet`.
Details on how the worklets work are given in Section \ref{sec:GeneratingCellSets:SingleType}.

.. todo:: Fix reference to generating cell sets.

Because the operation of this edge extraction depends only on :class:`vtkm::cont::CellSet` in a provided :class:`vtkm::cont::DataSet`, the filter class is a simple subclass of :class:`vtkm::filter::Filter`.

.. load-example:: ExtractEdgesFilterDeclaration
   :file: GuideExampleGenerateMeshConstantShape.cxx
   :caption: Header declaration for a data set filter.

The implementation of :func:`vtkm::filter::Filter::DoExecute` first gets the :class:`vtkm::cont::CellSet` and calls the worklet methods to generate a new :class:`vtkm::cont::CellSet` class.
It then uses a form of :func:`vtkm::filter::Filter::CreateResult` to generate the resulting :class:`vtkm::cont::DataSet`.

.. load-example:: ExtractEdgesFilterDoExecute
   :file: GuideExampleGenerateMeshConstantShape.cxx
   :caption: Implementation of the :func:`vtkm::filter::Filter::DoExecute` method of a data set filter.

The form of :func:`vtkm::filter::Filter::CreateResult` used (:exlineref:`ex:ExtractEdgesFilterDoExecute:CreateResult`) takes as input a :class:`vtkm::cont::CellSet` to use in the generated data.
In forms of :func:`vtkm::filter::Filter::CreateResult` used in previous examples of this chapter, the cell structure of the output was created from the cell structure of the input.
Because these cell structures were the same, coordinate systems and fields needed to be changed.
However, because we are providing a new :class:`vtkm::cont::CellSet`, we need to also specify how the coordinate systems and fields change.

The last two arguments to :func:`vtkm::filter::Filter::CreateResult` are providing this information.
The second-to-last argument is a ``std::vector`` of the :class:`vtkm::cont::CoordinateSystem`'s to use.
Because this filter does not actually change the points in the data set, the :class:`vtkm::cont::CoordinateSystem`'s can just be copied over.
The last argument provides a functor that maps a field from the input to the output.
The functor takes two arguments: the output :class:`vtkm::cont::DataSet` to modify and the input :class:`vtkm::cont::Field` to map.
In this example, the functor is defined as a lambda function (:exlineref:`ex:ExtractEdgesFilterDoExecute:FieldMapper`).

.. didyouknow::
   The field mapper in :numref:`ex:ExtractEdgesFilterDeclaration` uses a helper function named :func:`vtkm::filter::MapFieldPermutation`.
   In the case of this example, every cell in the output comes from one cell in the input.
   For this common case, the values in the field arrays just need to be permuted so that each input value gets to the right output value.
   :func:`vtkm::filter::MapFieldPermutation` will do this shuffling for you.

   |VTKm| also comes with a similar helper function :func:`vtkm::filter::MapFieldMergeAverage` that can be used when each output cell (or point) was constructed from multiple inputs.
   In this case, :func:`vtkm::filter::MapFieldMergeAverage` can do a simple average for each output value of all input values that contributed.

.. doxygenfunction:: vtkm::filter::MapFieldPermutation(const vtkm::cont::Field&, const vtkm::cont::ArrayHandle<vtkm::Id>&, vtkm::cont::Field&, vtkm::Float64)
.. doxygenfunction:: vtkm::filter::MapFieldPermutation(const vtkm::cont::Field&, const vtkm::cont::ArrayHandle<vtkm::Id>&, vtkm::cont::DataSet&, vtkm::Float64)
.. doxygenfunction:: vtkm::filter::MapFieldMergeAverage(const vtkm::cont::Field&, const vtkm::worklet::internal::KeysBase&, vtkm::cont::Field&)
.. doxygenfunction:: vtkm::filter::MapFieldMergeAverage(const vtkm::cont::Field&, const vtkm::worklet::internal::KeysBase&, vtkm::cont::DataSet&)

.. didyouknow::
   Although not the case in this example, sometimes a filter creating a new cell set changes the points of the cells.
   As long as the field mapper you provide to :func:`vtkm::filter::Filter::CreateResult` properly converts points from the input to the output, all fields and coordinate systems will be automatically filled in the output.
   Sometimes when creating this new cell set you also create new point coordinates for it.
   This might be because the point coordinates are necessary for the computation or might be due to a faster way of computing the point coordinates.
   In either case, if the filter already has point coordinates computed, it can use :func:`vtkm::filter::Filter::CreateResultCoordinateSystem` to use the precomputed point coordinates.


------------------------------
Data Set with Field Filters
------------------------------

.. index::
   double: filter; data set with field

Sometimes, a filter will generate a data set with a new cell set based off the cells of an input data set along with the data in at least one field.
For example, a field might determine how each cell is culled, clipped, or sliced.

In this section we provide an example implementation of a data set with field filter that blanks the cells in a data set based on a field that acts as a mask (or stencil).
Any cell associated with a mask value of zero will be removed.
For simplicity of this example, we will use the :class:`vtkm::filter::entity_extraction::Threshold` filter internally for the implementation.

.. load-example:: BlankCellsFilterDeclaration
   :file: GuideExampleFilterDataSetWithField.cxx
   :caption: Header declaration for a data set with field filter.

The implementation of :func:`vtkm::filter::Filter::DoExecute` first derives an array that contains a flag whether the input array value is zero or non-zero.
This is simply to guarantee the range for the threshold filter.
After that a threshold filter is set up and run to generate the result.

.. load-example:: BlankCellsFilterDoExecute
   :file: GuideExampleFilterDataSetWithField.cxx
   :caption: Implementation of the :func:`vtkm::filter::Filter::DoExecute` method of a data set with field filter.
