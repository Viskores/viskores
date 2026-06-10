=================================
Unknown Cell Sets
=================================

.. index::
   single: unknown cell set
   single: cell set; unknown

:class:`viskores::cont::DataSet` must hold a :class:`viskores::cont::CellSet` object, but it cannot know its specific type at compile time.
To manage storing :class:`viskores::cont::CellSet` objects without knowing their types, :class:`viskores::cont::DataSet` actually holds a reference using :class:`viskores::cont::UnknownCellSet`.
:class:`viskores::cont::UnknownCellSet` is a simple polymorphic container that stores a reference to a :class:`viskores::cont::CellSet` of unknown type.

.. doxygenclass:: viskores::cont::UnknownCellSet

It is possible to create an empty :class:`viskores::cont::UnknownCellSet`.
You can use the :func:`viskores::cont::UnknownCellSet::IsValid` function to query whether an :class:`viskores::cont::UnknownCellSet` holds a valid :class:`viskores::cont::CellSet`.
Performing operations on an invalid :class:`viskores::cont::UnknownCellSet` can lead to unexpected behavior.


------------------------------
Generic Operations
------------------------------

Some cell set operations in |Viskores| require a specific, concrete class of :class:`viskores::cont::CellSet`.
But :class:`viskores::cont::UnknownCellSet` provides several functions that allow you to operate on a cell set without knowing the exact type.

.. doxygenfunction:: viskores::cont::UnknownCellSet::IsValid
.. doxygenfunction:: viskores::cont::UnknownCellSet::GetCellSetBase()
.. doxygenfunction:: viskores::cont::UnknownCellSet::GetCellSetBase() const
.. doxygenfunction:: viskores::cont::UnknownCellSet::NewInstance
.. doxygenfunction:: viskores::cont::UnknownCellSet::GetCellSetName
.. doxygenfunction:: viskores::cont::UnknownCellSet::GetNumberOfCells
.. doxygenfunction:: viskores::cont::UnknownCellSet::GetNumberOfPoints
.. doxygenfunction:: viskores::cont::UnknownCellSet::GetCellShape
.. doxygenfunction:: viskores::cont::UnknownCellSet::GetNumberOfPointsInCell
.. doxygenfunction:: viskores::cont::UnknownCellSet::GetCellPointIds
.. doxygenfunction:: viskores::cont::UnknownCellSet::DeepCopyFrom
.. doxygenfunction:: viskores::cont::UnknownCellSet::PrintSummary
.. doxygenfunction:: viskores::cont::UnknownCellSet::ReleaseResourcesExecution


------------------------------
Casting to Known Types
------------------------------

.. index::
   double: unknown cell set; cast

There are many operations in |Viskores| that need to know the specific type of cell set.
To perform one of these types of operation, you need to retrieve the data as a :class:`viskores::cont::CellSet` concrete subclass.
If you happen to know (or can guess) the type, you can use the :func:`viskores::cont::UnknownCellSet::AsCellSet` method to retrieve the cell set as a specific type.
You can pass in a reference to a cell set object of the desired type to :func:`viskores::cont::UnknownCellSet::AsCellSet`.
You can also call :func:`viskores::cont::UnknownCellSet::AsCellSet` with no arguments and the cast cell set will be returned, but in this case you must specify the desired type with a template argument.

.. doxygenfunction:: viskores::cont::UnknownCellSet::AsCellSet(CellSetType& cellSet) const
.. doxygenfunction:: viskores::cont::UnknownCellSet::AsCellSet() const

.. load-example:: UnknownCellSetAsCellSet
   :file: GuideExampleDataSetCreation.cxx
   :caption: Retrieving a cell set of a known type from :class:`viskores::cont::UnknownCellSet`.

.. index::
   double: unknown cell set; query type

If the :class:`viskores::cont::UnknownCellSet` cannot store its cell set in the type given to :func:`viskores::cont::UnknownCellSet::AsCellSet`, it will throw an exception.
Thus, you should not use :func:`viskores::cont::UnknownCellSet::AsCellSet` with types that you are not sure about.
Use the :func:`viskores::cont::UnknownCellSet::CanConvert` method to determine if a given :class:`viskores::cont::CellSet` type will work with :func:`viskores::cont::UnknownCellSet::AsCellSet`.

.. doxygenfunction:: viskores::cont::UnknownCellSet::CanConvert

.. load-example:: UnknownCellSetCanConvert
   :file: GuideExampleDataSetCreation.cxx
   :caption: Querying whether a given :class:`viskores::cont::CellSet` can be retrieved from an :class:`viskores::cont::UnknownCellSet`.

By design, :func:`viskores::cont::UnknownCellSet::CanConvert` will return true for types that are not actually stored in the :class:`viskores::cont::UnknownCellSet` but can be retrieved.
If you need to know specifically what type is stored in the :class:`viskores::cont::UnknownCellSet`, you can use the :func:`viskores::cont::UnknownCellSet::IsType` method instead.
You can also use 
:func:`viskores::cont::UnknownCellSet::GetCellSetName` for debugging purposes.

.. doxygenfunction:: viskores::cont::UnknownCellSet::IsType

.. commonerrors::
   :func:`viskores::cont::UnknownCellSet::CanConvert` is almost always safer to use than :func:`viskores::cont::UnknownCellSet::IsType` or its similar methods.
   Even though :func:`viskores::cont::UnknownCellSet::IsType` reflects the actual cell set type, :func:`viskores::cont::UnknownCellSet::CanConvert` better describes how :class:`viskores::cont::UnknownCellSet` will behave.


----------------------------------------
Casting to a List of Potential Types
----------------------------------------

.. index::
   double: unknown cell set; cast

Using :func:`viskores::cont::UnknownCellSet::AsCellSet` is fine as long as the correct types are known, but often times they are not.
For this use case :class:`viskores::cont::UnknownCellSet` has a method named :func:`viskores::cont::UnknownCellSet::CastAndCallForTypes` that attempts to cast the cell set to some set of types.

.. doxygenfunction:: viskores::cont::UnknownCellSet::CastAndCallForTypes

The :func:`viskores::cont::UnknownCellSet::CastAndCallForTypes` method accepts a functor to run on the appropriately cast cell set.
The functor must have an overloaded const parentheses operator that accepts a :class:`viskores::cont::CellSet` of the appropriate type.
You also have to specify a template parameter that specifies a :class:`viskores::List` of cell set types to.
The macro :c:macro:`VISKORES_DEFAULT_CELL_SET_LIST` is often used when nothing more specific is known.
The macros :c:macro:`VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED` and :c:macro:`VISKORES_DEFAULT_CELL_SET_LIST_UNSTRUCTURED` are also useful when you want to operate on only structured or unstructured cell sets.

.. doxygendefine:: VISKORES_DEFAULT_CELL_SET_LIST
.. doxygendefine:: VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED
.. doxygendefine:: VISKORES_DEFAULT_CELL_SET_LIST_UNSTRUCTURED

.. load-example:: UnknownCellSetCastAndCallForTypes
   :file: GuideExampleDataSetCreation.cxx
   :caption: Operating with :func:`viskores::cont::UnknownCellSet::CastAndCallForTypes`.

.. didyouknow::
   The first (required) argument to :func:`viskores::cont::UnknownCellSet::CastAndCallForTypes` is the functor to call with the cell set.
   You can supply any number of optional arguments after that.
   Those arguments will be passed directly to the functor.
   This makes it easy to pass state to the functor.

.. didyouknow::
   When an :class:`viskores::cont::UnknownCellSet` is used in place of an :class:`viskores::cont::CellSet` as an argument to a worklet invocation, it will internally use :func:`viskores::cont::UnknownCellSet::CastAndCallForTypes` to attempt to call the worklet with an :class:`viskores::cont::CellSet` of the correct type.

:class:`viskores::cont::UnknownCellSet` has a simple subclass named :class:`viskores::cont::UncertainCellSet` for use when you can narrow the cell set to a finite set of types.
:class:`viskores::cont::UncertainCellSet` has a template parameter that must be specified: a :class:`viskores::List` of cell set types.
The :func:`viskores::cont::UncertainCellSet::CastAndCall` method behaves the same as :func:`viskores::cont::UnknownCellSet::CastAndCallForTypes` except that you do not have to specify the types to try.
Instead, the types are taken from the template parameters of the :class:`viskores::cont::UncertainCellSet` itself.

.. doxygenclass:: viskores::cont::UncertainCellSet
   :members:

.. load-example:: UncertainCellSet
   :file: GuideExampleDataSetCreation.cxx
   :caption: Using :class:`viskores::cont::UncertainCellSet` to cast and call a functor.

.. didyouknow::
   Like with :class:`viskores::cont::UnknownCellSet`, if an :class:`viskores::cont::UncertainCellSet` is used in a worklet invocation, it will internally use :func:`viskores::cont::UncertainCellSet::CastAndCall`.
   This provides a convenient way to specify what cell set types the invoker should try.

Both :class:`viskores::cont::UnknownCellSet` and :class:`viskores::cont::UncertainCellSet` provide a method named :func:`viskores::cont::UnknownCellSet::ResetCellSetList` to redefine the types to try.
It has a template parameter that is the :class:`viskores::List` of cell sets.
:func:`viskores::cont::UnknownCellSet::ResetCellSetList` returns a new :class:`viskores::cont::UncertainCellSet` with the given types.
This is a convenient way to pass these types to functions.

.. doxygenfunction:: viskores::cont::UnknownCellSet::ResetCellSetList() const
.. doxygenfunction:: viskores::cont::UnknownCellSet::ResetCellSetList(NewCellSetList) const

.. load-example:: UnknownCellSetResetCellSetList
   :file: GuideExampleDataSetCreation.cxx
   :caption: Resetting the types of an :class:`viskores::cont::UnknownCellSet`.

.. commonerrors::
   Because it returns a :class:`viskores::cont::UncertainCellSet`, you need to include :file:`viskores/cont/UncertainCellSet.h` if you use :func:`viskores::cont::UnknownCellSet::ResetCellSetList`.
   This is true even if you do not directly use the returned object.
