==============================
Worklet Arguments
==============================

From the ``ControlSignature`` and ``ExecutionSignature`` defined in worklets,
|Viskores| uses template metaprogramming to build the code required to manage
data from the control to the execution environment.
These signatures contain tags that define the meaning of each argument and
control how the argument data are transferred from the control to execution
environments and broken up for each worklet instance.

:chapref:`worklet-types:Worklet Types` documents the many ``ControlSignature``
and ``ExecutionSignature`` tags that come with the worklet types.
This chapter discusses the internals of these tags and how they control data
management.
Defining new worklet argument types can allow you to define new data structures
in |Viskores|.
New worklet arguments are also usually critical components for making new
worklet types, as described in the later chapter on creating new worklet types.

.. todo:: Reference this later chapter when it actually exists.

The management of data in worklet arguments is handled by three classes that
provide type checking, transportation, and fetching, respectively.
This chapter first describes these type-checking, transportation, and fetching
classes and then describes how ``ControlSignature`` and ``ExecutionSignature``
tags specify these classes.

Throughout this chapter, we demonstrate the definition of worklet arguments
using an example of a worklet argument that represents line segments in 2D.
The input for such an argument expects an
:class:`viskores::cont::ArrayHandle` containing floating-point
:type:`viskores::Vec` objects of size 2 to represent coordinates in the plane.
The values in the array are paired to define the two endpoints of each segment,
and the worklet instance receives a ``Vec``-2 of ``Vec``-2 objects representing
the two endpoints.
In practice, it is generally easier to use a
:class:`viskores::cont::ArrayHandleGroupVec` (see
:secref:`fancy-array-handles:Grouped Vector Arrays`), but this is a simple
example for demonstration purposes.
We also use this special worklet argument for our later example of a custom
worklet type.

------------------------------
Type Checks
------------------------------

.. index::
   single: type check

Before attempting to move data from the control to the execution environment,
the |Viskores| invokers check the input types to ensure that they are compatible
with the associated ``ControlSignature`` concept.
This is done with the :struct:`viskores::cont::arg::TypeCheck` structure.

.. doxygenstruct:: viskores::cont::arg::TypeCheck
   :members:

The :struct:`viskores::cont::arg::TypeCheck` structure is templated with two
parameters.
The first parameter is a tag that identifies which check to perform.
The second parameter is the type of the control argument (after any dynamic
casts).
The structure contains a static constant Boolean named ``value`` that is
``true`` if the type in the second parameter is compatible with the tag in the
first, or ``false`` otherwise.

Type checks are implemented with a defined type-check tag, which by convention
is defined in the :cpp:any:`viskores::cont::arg` namespace and starts with
``TypeCheckTag``, and a partial specialization of the
:struct:`viskores::cont::arg::TypeCheck` structure.
The following type checks, identified by their tags, are provided in
|Viskores|.

.. index::
   pair: type check; execution object

.. doxygenstruct:: viskores::cont::arg::TypeCheckTagExecObject

.. index::
   pair: type check; array

.. doxygenstruct:: viskores::cont::arg::TypeCheckTagArrayIn

.. doxygenstruct:: viskores::cont::arg::TypeCheckTagArrayOut

.. doxygenstruct:: viskores::cont::arg::TypeCheckTagArrayInOut

.. doxygenstruct:: viskores::cont::arg::TypeCheckTagAtomicArray

.. index::
   pair: type check; bit field

.. doxygenstruct:: viskores::cont::arg::TypeCheckTagBitField

.. index::
   pair: type check; cell set

.. doxygenstruct:: viskores::cont::arg::TypeCheckTagCellSet

.. doxygenstruct:: viskores::cont::arg::TypeCheckTagCellSetStructured

.. index::
   pair: type check; keys

.. doxygenstruct:: viskores::cont::arg::TypeCheckTagKeys

Here are some trivial examples of using
:struct:`viskores::cont::arg::TypeCheck`.
Typically these checks are done internally in the base |Viskores| invoker code,
so these examples are for demonstration only.

.. load-example:: TypeCheck
   :file: GuideExampleTransferringArguments.cxx
   :caption: Behavior of :struct:`viskores::cont::arg::TypeCheck`.

A type check is created by first defining a type-check tag object, which by
convention is placed in the :cpp:any:`viskores::cont::arg` namespace and
whose name starts with ``TypeCheckTag``.
Then, create a specialization of the
:struct:`viskores::cont::arg::TypeCheck` template class with the first template
argument matching the aforementioned tag.
As stated previously, the :struct:`viskores::cont::arg::TypeCheck` class must
contain a ``value`` static constant Boolean representing whether the type is
acceptable for the corresponding :class:`viskores::cont::Invoker` argument.

This example of a :struct:`viskores::cont::arg::TypeCheck` returns true for
control objects that are :class:`viskores::cont::ArrayHandle` objects with a
value type that is a floating-point :type:`viskores::Vec` of size 2.

.. load-example:: TypeCheckImpl
   :file: GuideExampleFractalWorklets.cxx
   :caption: Defining a custom :struct:`viskores::cont::arg::TypeCheck`.

------------------------------
Transport
------------------------------

.. index::
   single: transport

After all the argument types are checked, the |Viskores| dispatch mechanism
must load the data into the execution environment before scheduling a job to
run there.
This is done with the :struct:`viskores::cont::arg::Transport` structure.

.. doxygenstruct:: viskores::cont::arg::Transport
   :members:

The :struct:`viskores::cont::arg::Transport` structure is templated with three
parameters.
The first parameter is a tag that identifies which transport to perform.
The second parameter is the type of the control parameter (after any dynamic
casts).
The third parameter is a device adapter tag for the device on which the data
will be loaded.

A :struct:`viskores::cont::arg::Transport` contains a type named
``ExecObjectType`` that is the type used after data is moved to the execution
environment.
A :struct:`viskores::cont::arg::Transport` also has a ``const`` parenthesis
operator that takes five arguments: the control-side object to transport to the
execution environment, the control-side object that represents the input
domain, the size of the input domain, the size of the output domain, and a
reference to a :class:`viskores::cont::Token` object that defines the scope of
any generated execution-environment objects.
The parenthesis operator returns an execution-side object.
This operator is called in the control environment, and it returns an object
that is ready to be used in the execution environment.

Transports are implemented with a defined transport tag, which by convention
is defined in the :cpp:any:`viskores::cont::arg` namespace and starts with
``TransportTag``, and a partial specialization of the
:struct:`viskores::cont::arg::Transport` structure.
The following transports, identified by their tags, are provided in
|Viskores|.

.. index::
   pair: transport; execution object

.. doxygenstruct:: viskores::cont::arg::TransportTagExecObject

.. index::
   pair: transport; input array

.. doxygenstruct:: viskores::cont::arg::TransportTagArrayIn

.. index::
   pair: transport; output array

.. doxygenstruct:: viskores::cont::arg::TransportTagArrayOut

.. index::
   pair: transport; input/output array

.. doxygenstruct:: viskores::cont::arg::TransportTagArrayInOut

.. index::
   pair: transport; whole array input

.. doxygenstruct:: viskores::cont::arg::TransportTagWholeArrayIn

.. index::
   pair: transport; whole array output

.. doxygenstruct:: viskores::cont::arg::TransportTagWholeArrayOut

.. index::
   pair: transport; whole array input/output

.. doxygenstruct:: viskores::cont::arg::TransportTagWholeArrayInOut

.. index::
   pair: transport; atomic array

.. doxygenstruct:: viskores::cont::arg::TransportTagAtomicArray

.. index::
   pair: transport; bit field

.. doxygenstruct:: viskores::cont::arg::TransportTagBitFieldIn

.. doxygenstruct:: viskores::cont::arg::TransportTagBitFieldOut

.. doxygenstruct:: viskores::cont::arg::TransportTagBitFieldInOut

.. index::
   pair: transport; cell set

.. doxygenstruct:: viskores::cont::arg::TransportTagCellSetIn

.. index::
   pair: transport; topology mapped field

.. doxygenstruct:: viskores::cont::arg::TransportTagTopologyFieldIn

.. index::
   pair: transport; keys

.. doxygenstruct:: viskores::cont::arg::TransportTagKeysIn

.. index::
   pair: transport; input array keyed values

.. doxygenstruct:: viskores::cont::arg::TransportTagKeyedValuesIn

.. index::
   pair: transport; output array keyed values

.. doxygenstruct:: viskores::cont::arg::TransportTagKeyedValuesOut

.. index::
   pair: transport; input/output array keyed values

.. doxygenstruct:: viskores::cont::arg::TransportTagKeyedValuesInOut

Here are some trivial examples of using
:struct:`viskores::cont::arg::Transport`.
Typically this movement is done internally in the |Viskores| dispatching code,
so these examples are for demonstration only.

.. load-example:: Transport
   :file: GuideExampleTransferringArguments.cxx
   :caption: Behavior of :struct:`viskores::cont::arg::Transport`.

A transport is created by first defining a transport-tag object, which by
convention is placed in the :cpp:any:`viskores::cont::arg` namespace and
whose name starts with ``TransportTag``.
Then, create a specialization of the
:struct:`viskores::cont::arg::Transport` template class with the first template
argument matching the aforementioned tag.
As stated previously, the :struct:`viskores::cont::arg::Transport` class must
contain an ``ExecObjectType`` type and a parenthesis operator that turns the
associated control argument into an execution-environment object.

This example internally uses a
:class:`viskores::cont::ArrayHandleGroupVec` to take values from an input
:class:`viskores::cont::ArrayHandle` and pair them to represent line segments.
The resulting execution object is an array portal containing ``Vec``-2 values
of ``Vec``-2 objects.

.. load-example:: TransportImpl
   :file: GuideExampleFractalWorklets.cxx
   :caption: Defining a custom :struct:`viskores::cont::arg::Transport`.

.. commonerrors::

   It is fair to assume that the :struct:`viskores::cont::arg::Transport` control-object type matches
   whatever the associated :struct:`viskores::cont::arg::TypeCheck` allows.
   However, it is good practice to provide a secondary compile-time check in the :struct:`viskores::cont::arg::Transport` class, like the one at :exlineref:`ex:TransportImpl:CheckControlObject`, for debugging in case there is a problem with the :struct:`viskores::cont::arg::TypeCheck` or this :struct:`viskores::cont::arg::Transport` is used with an unexpected :struct:`viskores::cont::arg::TypeCheck`.


------------------------------
Fetch
------------------------------

.. index::
   single: fetch

Before a worklet function is invoked, the |Viskores| internals pull the
appropriate data out of the execution object and pass it to the worklet
function.
A class named :struct:`viskores::exec::arg::Fetch` is responsible for pulling
this data out of execution objects and putting computed data into them.

.. doxygenstruct:: viskores::exec::arg::Fetch
   :members:

The :struct:`viskores::exec::arg::Fetch` structure is templated with three
parameters.
The first parameter is a tag that identifies which type of fetch to perform.
The second parameter is a different tag that identifies the aspect of the data
to fetch.

The third template parameter to a :struct:`viskores::exec::arg::Fetch` is the
type of execution object created by the
:struct:`viskores::cont::arg::Transport`, as described in
:secref:`worklet-arguments:Transport`.
This is generally where the data are fetched from.

A :struct:`viskores::exec::arg::Fetch` also has a pair of methods named
``Load()`` and ``Store()`` that get data from and add data to the execution object
at a given domain or thread index.

.. index::
   single: thread indices

In both the ``Load()`` and ``Store()`` methods, the first parameter is a thread
indices object that manages the multiple indices relevant to the worklet
invocation, including the input index, output index, and visit index, all of
which can be different.

The specific type of the thread indices object depends on the type of worklet
being invoked, but all thread indices classes implement methods named
``GetInputIndex``, ``GetOutputIndex``, and ``GetVisitIndex`` to get those
respective indices.
The thread indices object may also contain other methods to get information
pertinent to the associated worklet's execution.
For example, a thread indices object associated with a topology map has methods
to get the shape identifier and incident-from indices of the current input
object.
Thread indices objects are discussed in more detail in the later section on
thread indices.

.. todo:: Add reference to section once is is created.

.. index::
   single: aspect
   pair: fetch; aspect

Fetches are specified with a pair of fetch and aspect tags.
Fetch tags are by convention defined in the
:cpp:any:`viskores::exec::arg` namespace and start with ``FetchTag``.
Likewise, aspect tags are also defined in the
:cpp:any:`viskores::exec::arg` namespace and start with ``AspectTag``.
The :struct:`viskores::exec::arg::Fetch` class is partially specialized on
these two tags.

The most common aspect tag is
:struct:`viskores::exec::arg::AspectTagDefault`, and all fetch tags should have
a specialization of :struct:`viskores::exec::arg::Fetch` with this tag.
The following list of fetch tags describes the execution objects they work
with and the data they pull for each aspect tag they support.

Fetch Tags
================

.. index::
   pair: aspect; default

.. index::
   pair: fetch; execution object

.. doxygenstruct:: viskores::exec::arg::FetchTagExecObject

.. index::
   pair: fetch; whole cell set

.. doxygenstruct:: viskores::exec::arg::FetchTagWholeCellSetIn

.. index::
   pair: fetch; direct input array

.. doxygenstruct:: viskores::exec::arg::FetchTagArrayDirectIn

.. index::
   pair: fetch; direct output array

.. doxygenstruct:: viskores::exec::arg::FetchTagArrayDirectOut

.. doxygenstruct:: viskores::exec::arg::FetchTagArrayDirectInOut

.. index::
   pair: fetch; cell set

.. doxygenstruct:: viskores::exec::arg::FetchTagCellSetIn

.. index::
   pair: fetch; topology map array input

.. doxygenstruct:: viskores::exec::arg::FetchTagArrayTopologyMapIn

A fetch is created by first defining a fetch-tag object, which by convention is
placed in the :cpp:any:`viskores::exec::arg` namespace and whose name starts
with ``FetchTag``.
Then, create a specialization of the :struct:`viskores::exec::arg::Fetch`
template class with the first template argument matching the aforementioned
tag.
As stated previously, the :struct:`viskores::exec::arg::Fetch` class must
contain a pair of ``Load()`` and ``Store()`` methods that get a value out of the
data and store a value in the data, respectively.


.. load-example:: FetchImplBasic
   :file: GuideExampleFractalWorklets.cxx
   :caption: Defining a custom :struct:`viskores::exec::arg::Fetch`.

.. didyouknow::

   The fetch defined in :numref:`ex:FetchImplBasic` could actually be replaced
   by the more general :struct:`viskores::exec::arg::FetchTagArrayDirectIn`
   that already comes with |Viskores|.
   This example is provided mostly for demonstrative purposes.

Aspect Tags
================

In addition to the aforementioned aspect tags that are explicitly paired with
fetch tags, |Viskores| also provides some aspect tags that either modify the
behavior of a general fetch or simply ignore the type of fetch.
These descriptions are notional as any :struct:`viskores::exec::arg::Fetch` is free to interpret the aspect however it likes.

.. index::
   pair: aspect; default

.. doxygenstruct:: viskores::exec::arg::AspectTagDefault

.. index::
   pair: aspect; work index

.. doxygenstruct:: viskores::exec::arg::AspectTagWorkIndex

.. index::
   pair: aspect; input index

.. doxygenstruct:: viskores::exec::arg::AspectTagInputIndex

.. index::
   pair: aspect; output index

.. doxygenstruct:: viskores::exec::arg::AspectTagOutputIndex

.. index::
   pair: aspect; visit index

.. doxygenstruct:: viskores::exec::arg::AspectTagVisitIndex

.. index::
   pair: aspect; cell shape

.. doxygenstruct:: viskores::exec::arg::AspectTagCellShape

.. index::
   pair: aspect; incident element count

.. doxygenstruct:: viskores::exec::arg::AspectTagIncidentElementCount

.. index::
   pair: aspect; incident indices

.. doxygenstruct:: viskores::exec::arg::AspectTagIncidentElementIndices

.. index::
   pair: aspect; value count

.. doxygenstruct:: viskores::exec::arg::AspectTagValueCount

An aspect is created by first defining an aspect-tag object, which by convention
is placed in the :cpp:any:`viskores::exec::arg` namespace and whose name
starts with ``AspectTag``.
Then, create specializations of the :struct:`viskores::exec::arg::Fetch`
template class where appropriate, with the second template argument matching
the aforementioned tag.

This example creates a specialization of a
:struct:`viskores::exec::arg::Fetch` to retrieve the first point of a line
segment.

.. load-example:: AspectImpl
   :file: GuideExampleFractalWorklets.cxx
   :caption: Defining a custom aspect.

--------------------------------------------
Creating New ``ControlSignature`` Tags
--------------------------------------------

.. index::
   pair: control signature; tags
   triple: signature; control; tags

The type checks, transports, and fetches defined in the previous sections of
this chapter conspire to interpret the arguments given to a
:class:`viskores::cont::Invoker` and provide data to an instance of a worklet.
What remains to be defined are the tags used in the ``ControlSignature`` and
``ExecutionSignature`` that bring these three items together.
These two types of tags are defined differently.
This section discusses the ``ControlSignature`` tags.

A ``ControlSignature`` tag is defined by a ``struct`` (or, equivalently, a
``class``).
This structure is typically defined inside a worklet, or more typically a
worklet superclass, so that it can be used without qualifying its namespace.
|Viskores| has requirements for every defined ``ControlSignature`` tag.

The first requirement is that a ``ControlSignature`` tag must inherit from
:struct:`viskores::cont::arg::ControlSignatureTagBase`.
You will get a compile error if you attempt to use a type that is not a
subclass of :struct:`viskores::cont::arg::ControlSignatureTagBase` in a
``ControlSignature``.

.. doxygenstruct:: viskores::cont::arg::ControlSignatureTagBase
   :members:

The second requirement is that a ``ControlSignature`` tag must contain the
following three types: ``TypeCheckTag``, ``TransportTag``, and ``FetchTag``.
As the names imply, these specify tags for
:struct:`viskores::cont::arg::TypeCheck`,
:struct:`viskores::cont::arg::Transport`, and
:struct:`viskores::exec::arg::Fetch`, respectively, which were discussed
earlier in this chapter.

The following example defines a ``ControlSignature`` tag for an array that
represents 2D line segments using the classes defined in previous examples.

.. load-example:: CustomControlSignatureTag
   :file: GuideExampleFractalWorklets.cxx
   :caption: Defining a new ``ControlSignature`` tag.

Once defined, this tag can be used like any other ``ControlSignature`` tag.

.. load-example:: UseCustomControlSignatureTag
   :file: GuideExampleFractalWorklets.cxx
   :caption: Using a custom ``ControlSignature`` tag.

----------------------------------------------
Creating New ``ExecutionSignature`` Tags
----------------------------------------------

.. index::
   pair: execution signature; tags
   triple: signature; execution; tags

An ``ExecutionSignature`` tag is defined by a ``struct`` (or, equivalently, a
``class``).
This structure is typically defined inside a worklet, or more typically a
worklet superclass, so that it can be used without qualifying its namespace.
|Viskores| has requirements for every defined ``ExecutionSignature`` tag.

The first requirement is that an ``ExecutionSignature`` tag must inherit from
:struct:`viskores::exec::arg::ExecutionSignatureTagBase`.
You will get a compile error if you attempt to use a type that is not a
subclass of :struct:`viskores::exec::arg::ExecutionSignatureTagBase` in an
``ExecutionSignature``.

.. doxygenstruct:: viskores::exec::arg::ExecutionSignatureTagBase
   :members:

The second requirement is that an ``ExecutionSignature`` tag must contain a
type named ``AspectTag``, which is set to an aspect tag.
As discussed in :secref:`worklet-arguments:Fetch`, the aspect tag is passed as
a template argument to the :struct:`viskores::exec::arg::Fetch` class to modify
the data it loads and stores.
The numerical ``ExecutionSignature`` tags (that is, ``_1``, ``_2``, and so on)
operate by setting ``AspectTag`` to
:struct:`viskores::exec::arg::AspectTagDefault`, effectively engaging the
default fetch.

.. index::
   pair: aspect; default

The third requirement is that an ``ExecutionSignature`` tag contain an
``INDEX`` member that is a ``static const`` :type:`viskores::IdComponent`.
The number to which ``INDEX`` is set refers to the ``ControlSignature``
argument from which that data come, indexed starting at 1.
The numerical ``ExecutionSignature`` tags (that is, ``_1``, ``_2``, and so on)
operate by setting their ``INDEX`` values to the corresponding number (1, 2,
and so on).
An ``ExecutionSignature`` tag might take another tag as a template argument and
copy the ``INDEX`` from one to the other.
This allows you to use a tag to modify the aspect of another tag.
Most often, this is used to apply a particular aspect to a numerical
``ExecutionSignature`` tag (that is, ``_1``, ``_2``, and so on).
Still other ``ExecutionSignature`` tags might not need direct access to any
``ControlSignature`` arguments, such as those that pull information from
thread indices.
If ``INDEX`` does not matter because the execution-object parameter to the
:struct:`viskores::exec::arg::Fetch` ``Load()`` and ``Store()`` methods is ignored,
the ``ExecutionSignature`` tag can set ``INDEX`` to 1 because there is
guaranteed to be at least one control argument.

The following example defines an ``ExecutionSignature`` tag to get the
coordinates for only the first point in a 2D line segment.
The defined tag takes another tag as an argument, generally one of the numeric
tags, which is expected to point to a ``ControlSignature`` argument with a
``LineSegment2DCoordinatesIn`` tag, as defined in
:numref:`ex:CustomControlSignatureTag`.

.. load-example:: CustomExecutionSignatureTag
   :file: GuideExampleFractalWorklets.cxx
   :caption: Defining a new ``ExecutionSignature`` tag.

Once defined, this tag can be used like any other ``ExecutionSignature`` tag.

.. load-example:: UseCustomExecutionSignatureTag
   :file: GuideExampleFractalWorklets.cxx
   :caption: Using a custom ``ExecutionSignature`` tag.
