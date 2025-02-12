==============================
Running Filters
==============================

.. index:: filter

Filters are functional units that take data as input and write new data as output.
Filters operate on :class:`viskores::cont::DataSet` objects, which are described in :chapref:`dataset:Data Sets`.

.. didyouknow::
   The structure of filters in |Viskores| is significantly simpler than their counterparts in VTK.
   VTK filters are arranged in a dataflow network (a.k.a. a visualization pipeline) and execution management is handled automatically.
   In contrast, |Viskores| filters are simple imperative units, which are simply called with input data and return output data.

|Viskores| comes with several filters ready for use.
This chapter gives an overview of how to run the filters.
:chapref:`provided-filters:Provided Filters` describes the common filters provided by |Viskores|.
Later, :partref:`part-developing:Developing Algorithms` describes the necessary steps in creating new filters in |Viskores|.


------------------------------
Basic Filter Operation
------------------------------

Different filters will be used in different ways, but the basic operation of all filters is to instantiate the filter class, set the state parameters on the filter object, and then call the filter's :func:`viskores::filter::Filter::Execute` method.
It takes a :class:`viskores::cont::DataSet` and returns a new :class:`viskores::cont::DataSet`, which contains the modified data.

.. doxygenfunction:: viskores::filter::Filter::Execute(const viskores::cont::DataSet&)

The :func:`viskores::filter::Filter::Execute` method can alternately take a :class:`viskores::cont::PartitionedDataSet` object, which is a composite of :class:`viskores::cont::DataSet` objects.
In this case :func:`viskores::filter::Filter::Execute` will return another :class:`viskores::cont::PartitionedDataSet` object.

.. doxygenfunction:: viskores::filter::Filter::Execute(const viskores::cont::PartitionedDataSet&)

The following example provides a simple demonstration of using a filter.
It specifically uses the point elevation filter to estimate the air pressure at each point based on its elevation.

.. load-example:: PointElevation
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`viskores::filter::field_transform::PointElevation` to estiate air pressure.

We see that this example follows the previously described procedure of constructing the filter (:exlineref:`line %s<PointElevation:Construct>`), setting the state parameters (:exlineref:`lines %s<PointElevation:SetStateStart>` :exlineref:`-- %s<PointElevation:SetStateEnd>`), and finally executing the filter on a :class:`viskores::cont::DataSet` (:exlineref:`line %s<PointElevation:Execute>`).

.. index:: field

Every :class:`viskores::cont::DataSet` object contains a list of *fields*, which describe some numerical value associated with different parts of the data set in space.
Fields often represent physical properties such as temperature, pressure, or velocity.
Fields are identified with string names.
There are also special fields called coordinate systems that describe the location of points in space.
Field are mentioned here because they are often used as input data to the filter's operation and filters often generate new fields in the output.
This is the case in :numref:`ex:PointElevation`.
In :exlineref:`line %s<PointElevation:SetInputField>` the coordinate system is set as the input field and in :exlineref:`line %s<PointElevation:SetOutputField>` the name to use for the generated output field is selected.


------------------------------
Advanced Field Management
------------------------------

.. index::
   double: filter; fields

Most filters work with fields as inputs and outputs to their algorithms.
Although in the previous discussions of the filters we have seen examples of specifying fields, these examples have been kept brief in the interest of clarity.
In this section we revisit how filters manage fields and provide more detailed documentation of the controls.

Note that not all of the discussion in this section applies to all the filters provided by |Viskores|.
For example, not all filters have a specified input field.
But where possible, the interface to the filter objects is kept consistent.

Input Fields
==============================

.. index::
   triple: filter; input; fields

Filters that take one or more fields as input have a common set of methods to set the "active" fields to operate on.
They might also have custom methods to ease setting the appropriate fields, but these are the base methods.

.. doxygenfunction:: viskores::filter::Filter::SetActiveField(const std::string&, viskores::cont::Field::Association)

.. doxygenfunction:: viskores::filter::Filter::SetActiveField(viskores::IdComponent, const std::string&, viskores::cont::Field::Association)

.. doxygenfunction:: viskores::filter::Filter::GetActiveFieldName

.. doxygenfunction:: viskores::filter::Filter::GetActiveFieldAssociation

.. doxygenfunction:: viskores::filter::Filter::SetActiveCoordinateSystem(viskores::Id)

.. doxygenfunction:: viskores::filter::Filter::SetActiveCoordinateSystem(viskores::IdComponent, viskores::Id)

.. doxygenfunction:: viskores::filter::Filter::GetActiveCoordinateSystemIndex

.. doxygenfunction:: viskores::filter::Filter::SetUseCoordinateSystemAsField(bool)

.. doxygenfunction:: viskores::filter::Filter::SetUseCoordinateSystemAsField(viskores::IdComponent, bool)

.. doxygenfunction:: viskores::filter::Filter::GetUseCoordinateSystemAsField

.. doxygenfunction:: viskores::filter::Filter::GetNumberOfActiveFields

The :func:`viskores::filter::Filter::SetActiveField` method takes an optional argument that specifies which topological elements the field is associated with (such as points or cells).
The :enum:`viskores::cont::Field::Association` enumeration is used to select the field association.

.. load-example:: SetActiveFieldWithAssociation
   :file: GuideExampleProvidedFilters.cxx
   :caption: Setting a field's active filter with an association.

.. commonerrors::
   It is possible to have two fields with the same name that are only differentiable by the association.
   That is, you could have a point field and a cell field with different data but the same name.
   Thus, it is best practice to specify the field association when possible.
   Likewise, it is poor practice to have two fields with the same name, particularly if the data are not equivalent in some way.
   It is often the case that fields are selected without an association.

It is also possible to set the active scalar field as a coordinate system of the data.
A coordinate system essentially provides the spatial location of the points of the data and they have a special place in the :class:`viskores::cont::DataSet` structure.
(See :secref:`dataset:Coordinate Systems` for details on coordinate systems.)
You can use a coordinate system as the active scalars by calling the :func:`viskores::filter::Filter::SetUseCoordinateSystemAsField` method with a true flag.
Since a :class:`viskores::cont::DataSet` can have multiple coordinate systems, you can select the desired coordinate system with :func:`viskores::filter::Filter::SetActiveCoordinateSystem`.
(By default, the first coordinate system, index 0, will be used.)

.. load-example:: SetCoordinateSystem
   :file: GuideExampleProvidedFilters.cxx
   :caption: Setting the active coordinate system.

Passing Fields from Input to Output
========================================

.. index::
   triple: filter; passing; fields

After a filter successfully executes and returns a new data set, fields are mapped from input to output.
Depending on what operation the filter does, this could be a simple shallow copy of an array, or it could be a computed operation.
By default, the filter will automatically pass all fields from input to output (performing whatever transformations are necessary).
You can control which fields are passed (and equivalently which are not) with the :func:`viskores::filter::Filter::SetFieldsToPass` methods.

.. doxygenfunction:: viskores::filter::Filter::SetFieldsToPass(viskores::filter::FieldSelection&&)

.. doxygenfunction:: viskores::filter::Filter::GetFieldsToPass() const
.. doxygenfunction:: viskores::filter::Filter::GetFieldsToPass()

There are multiple ways to to use :func:`viskores::filter::Filter::SetFieldsToPass` to control what fields are passed.
If you want to turn off all fields so that none are passed, call :func:`viskores::filter::Filter::SetFieldsToPass` with :enum:`viskores::filter::FieldSelection::Mode::None`.

.. load-example:: PassNoFields
   :file: GuideExampleProvidedFilters.cxx
   :caption: Turning off the passing of all fields when executing a filter.

If you want to pass one specific field, you can pass that field's name to :func:`viskores::filter::Filter::SetFieldsToPass`.

.. doxygenfunction:: viskores::filter::Filter::SetFieldsToPass(const std::string&, viskores::filter::FieldSelection::Mode)
.. doxygenfunction:: viskores::filter::Filter::SetFieldsToPass(const std::string&, viskores::cont::Field::Association, viskores::filter::FieldSelection::Mode)

.. load-example:: PassOneField
   :file: GuideExampleProvidedFilters.cxx
   :caption: Setting one field to pass by name.

Or you can provide a list of fields to pass by giving :func:`viskores::filter::Filter::SetFieldsToPass` an initializer list of names.

.. doxygenfunction:: viskores::filter::Filter::SetFieldsToPass(std::initializer_list<std::string>, viskores::filter::FieldSelection::Mode)

.. load-example:: PassListOfFields
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using a list of fields for a filter to pass.

If you want to instead select a list of fields to *not* pass, you can add :enum:`viskores::filter::FieldSelection::Mode::Exclude` as an argument to :func:`viskores::filter::Filter::SetFieldsToPass`.

.. load-example:: PassExcludeFields
   :file: GuideExampleProvidedFilters.cxx
   :caption: Excluding a list of fields for a filter to pass.

Ultimately, :func:`viskores::filter::Filter::SetFieldsToPass` takes a :class:`viskores::filter::FieldSelection` object.
You can create one directly to select (or exclude) specific fields and their associations.

.. doxygenclass:: viskores::filter::FieldSelection
   :members:

.. load-example:: FieldSelection
   :file: GuideExampleProvidedFilters.cxx
   :caption: Using :class:`viskores::filter::FieldSelection` to select cells to pass.

It is also possible to specify field attributions directly to :func:`viskores::filter::Filter::SetFieldsToPass`.
If you only have one field, you can just specify both the name and attribution.
If you have multiple fields, you can provide an initializer list of ``std::pair`` or :class:`viskores::Pair` containing a ``std::string`` and a :enum:`viskores::cont::Field::Association`.
In either case, you can add an optional last argument of :enum:`viskores::filter::FieldSelection::Mode::Exclude` to exclude the specified filters instead of selecting them.

.. doxygenfunction:: viskores::filter::Filter::SetFieldsToPass(std::initializer_list<std::pair<std::string, viskores::cont::Field::Association>>, viskores::filter::FieldSelection::Mode)

.. load-example:: PassFieldAndAssociation
   :file: GuideExampleProvidedFilters.cxx
   :caption: Selecting one field and its association for a filter to pass.

.. load-example:: PassListOfFieldsAndAssociations
   :file: GuideExampleProvidedFilters.cxx
   :caption: Selecting a list of fields and their associations for a filter to pass.

Note that coordinate systems in a :class:`viskores::cont::DataSet` are simply links to point fields, and by default filters will pass coordinate systems regardless of the field selection flags.
To prevent a filter from passing a coordinate system if its associated field is not selected, use the :func:`viskores::filter::Filter::SetPassCoordinateSystems` method.

.. doxygenfunction:: viskores::filter::Filter::SetPassCoordinateSystems

.. doxygenfunction:: viskores::filter::Filter::GetPassCoordinateSystems

.. load-example:: PassNoCoordinates
   :file: GuideExampleProvidedFilters.cxx
   :caption: Turning off the automatic selection of fields associated with a :class:`viskores::cont::DataSet`'s coordinate system.

Output Field Names
==============================

Many filters will create fields of data.
A common way to set the name of the output field is to use the :func:`viskores::filter::Filter::SetOutputFieldName` method.

.. doxygenfunction:: viskores::filter::Filter::SetOutputFieldName

.. doxygenfunction:: viskores::filter::Filter::GetOutputFieldName

Most filters will have a default name to use for its generated fields.
It is also common for filters to provide convenience methods to name the output fields.
