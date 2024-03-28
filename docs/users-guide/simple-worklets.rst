==============================
Simple Worklets
==============================

.. index:: worklet; creating

The simplest way to implement an algorithm in |VTKm| is to create a *worklet*.
A worklet is fundamentally a functor that operates on an element of data.
Thus, it is a ``class`` or ``struct`` that has an overloaded parenthesis operator (which must be declared ``const`` for thread safety).
However, worklets are also embedded with a significant amount of metadata on how the data should be managed and how the execution should be structured.

.. load-example:: SimpleWorklet
   :file: GuideExampleSimpleAlgorithm.cxx
   :caption: A simple worklet.

As can be seen in :numref:`ex:SimpleWorklet`, a worklet is created by implementing a ``class`` or ``struct`` with the following features.

.. index::
   single: control signature
   single: signature; control
   single: execution signature
   single: signature; execution
   single: input domain

1. The class must publicly inherit from a base worklet class that specifies the type of operation being performed (:exlineref:`ex:SimpleWorklet:Inherit`).
2. The class must contain a functional type named ``ControlSignature`` (:exlineref:`ex:SimpleWorklet:ControlSignature`), which specifies what arguments are expected when invoking the class in the control environment.
3. The class must contain a functional type named ``ExecutionSignature`` (:exlineref:`ex:SimpleWorklet:ExecutionSignature`), which specifies how the data gets passed from the arguments in the control environment to the worklet running in the execution environment.
4. The class specifies an ``InputDomain`` (:exlineref:`ex:SimpleWorklet:InputDomain`), which identifies which input parameter defines the input domain of the data.
5. The class must contain an implementation of the parenthesis operator, which is the method that is executed in the execution environment (lines :exlineref:`{line}<ex:SimpleWorklet:OperatorStart>`--:exlineref:`{line}<ex:SimpleWorklet:OperatorEnd>`).
   The parenthesis operator must be declared ``const``.


------------------------------
Control Signature
------------------------------

.. index::
   single: control signature
   single: signature; control
   single: worklet; control signature

The control signature of a worklet is a functional type named ``ControlSignature``.
The function prototype matches what data are provided when the worklet is invoked (as described in :secref:`simple-worklets:Invoking a Worklet`).

.. load-example:: ControlSignature
   :file: GuideExampleSimpleAlgorithm.cxx
   :caption: A ``ControlSignature``.

.. didyouknow::
   If the code in :numref:`ex:ControlSignature` looks strange, you may be unfamiliar with :index:`function types`.
   In C++, functions have types just as variables and classes do.
   A function with a prototype like

   ``void functionName(int arg1, float arg2);``

   has the type ``void(int, float)``.
   |VTKm| uses function types like this as a :index:`signature` that defines the structure of a function call.

.. index:: signature; tags

The return type of the function prototype is always ``void``.
The parameters of the function prototype are *tags* that identify the type of data that is expected to be passed to invoke.
``ControlSignature`` tags are defined by the worklet type and the various
tags are documented more fully in :chapref:`worklet-types:Worklet Types`.
In the case of :numref:`ex:ControlSignature`, the two tags ``FieldIn`` and ``FieldOut`` represent input and output data, respectively.

.. index::
   single: control signature
   single: signature; control

By convention, ``ControlSignature`` tag names start with the base concept (e.g. ``Field`` or ``Topology``) followed by the domain (e.g. ``Point`` or ``Cell``) followed by ``In`` or ``Out``.
For example, ``FieldPointIn`` would specify values for a field on the points of a mesh that are used as input (read only).
Although they should be there in most cases, some tag names might leave out the domain or in/out parts if they are obvious or ambiguous.


------------------------------
Execution Signature
------------------------------

.. index::
   single: execution signature
   single: signature; execution
   single: worklet; execution signature

Like the control signature, the execution signature of a worklet is a functional type named ``ExecutionSignature``.
The function prototype must match the parenthesis operator (described in :secref:`simple-worklets:Worklet Operator`) in terms of arity and argument semantics.

.. load-example:: ExecutionSignature
   :file: GuideExampleSimpleAlgorithm.cxx
   :caption: An ``ExecutionSignature``.

The arguments of the ``ExecutionSignature``'s function prototype are tags that define where the data come from.
The most common tags are an underscore followed by a number, such as ``_1``, ``_2``, etc.
These numbers refer back to the corresponding argument in the ``ControlSignature``.
For example, ``_1`` means data from the first control signature argument, ``_2`` means data from the second control signature argument, etc.

Unlike the control signature, the execution signature optionally can declare a return type if the parenthesis operator returns a value.
If this is the case, the return value should be one of the numeric tags (i.e. ``_1``, ``_2``, etc.)
to refer to one of the data structures of the control signature.
If the parenthesis operator does not return a value, then ``ExecutionSignature`` should declare the return type as ``void``.

In addition to the numeric tags, there are other execution signature tags to represent other types of data.
For example, the ``WorkIndex`` tag identifies the instance of the worklet invocation.
Each call to the worklet function will have a unique ``WorkIndex``.
Other such tags exist and are described in the following section on worklet types where appropriate.


------------------------------
Input Domain
------------------------------

.. index::
   single: input domain
   single: worklet; input domain

All worklets represent data parallel operations that are executed over independent elements in some domain.
The type of domain is inherent from the worklet type, but the size of the domain is dependent on the data being operated on.

A worklet identifies the argument specifying the domain with a type alias named ``InputDomain``.
The ``InputDomain`` must be aliased to one of the execution signature numeric tags (i.e. ``_1``, ``_2``, etc.).
By default, the ``InputDomain`` points to the first argument, but a worklet can override that to point to any argument.

.. load-example:: InputDomain
   :file: GuideExampleSimpleAlgorithm.cxx
   :caption: An ``InputDomain`` declaration.

Different types of worklets can have different types of domain.
For example a simple field map worklet has a ``FieldIn`` argument as its input domain, and the size of the input domain is taken from the size of the associated field array.
Likewise, a worklet that maps topology has a ``CellSetIn`` argument as its input domain, and the size of the input domain is taken from the cell set.

Specifying the ``InputDomain`` is optional.
If it is not specified, the first argument is assumed to be the input domain.


------------------------------
Worklet Operator
------------------------------

A worklet is fundamentally a functor that operates on an element of data.
Thus, the algorithm that the worklet represents is contained in or called from the parenthesis operator method.

.. load-example:: WorkletOperator
   :file: GuideExampleSimpleAlgorithm.cxx
   :caption: An overloaded parenthesis operator of a worklet.

There are some constraints on the parenthesis operator.
First, it must have the same arity as the ``ExecutionSignature``, and the types of the parameters and return must be compatible.
Second, because it runs in the execution environment, it must be declared with the ``VTKM_EXEC`` (or ``VTKM_EXEC_CONT``) modifier.
Third, the method must be declared ``const`` to help preserve thread safety.


------------------------------
Invoking a Worklet
------------------------------

.. index:: worklet; invoke

Previously in this chapter we discussed creating a simple worklet.
In this section we describe how to run the worklet in parallel.

A worklet is run using the :class:`vtkm::cont::Invoker` class.

.. load-example:: WorkletInvoke
   :file: GuideExampleSimpleAlgorithm.cxx
   :caption: Invoking a worklet.

Using an :class:`vtkm::cont::Invoker` is simple.
First, an :class:`vtkm::cont::Invoker` can be simply constructed with no arguments (:exlineref:`ex:WorkletInvoke:Construct`).
Next, the :class:`vtkm::cont::Invoker` is called as if it were a function (:exlineref:`ex:WorkletInvoke:Call`).

The first argument to the invoke is always an instance of the worklet.
The remaining arguments are data that are passed (indirectly) to the worklet.
Each of these arguments (after the worklet) match a corresponding argument listed in the ``ControlSignature``.
So in the invocation in :exlineref:`ex:WorkletInvoke:Call`, the second and third arguments correspond the the two ``ControlSignature`` arguments given in :numref:`ex:ControlSignature`.
``psiArray`` corresponds to the ``FieldIn`` argument and ``nmsArray`` corresponds to the ``FieldOut`` argument.

.. doxygenstruct:: vtkm::cont::Invoker
   :members:


----------------------------------------
Preview of More Complex Worklets
----------------------------------------

This chapter demonstrates the creation of a worklet that performs a very simple math operation in parallel.
However, we have just scratched the surface of the kinds of algorithms that can be expressed with |VTKm| worklets.
There are many more execution patterns and data handling constructs.
The following example gives a preview of some of the more advanced features of worklets.

.. load-example:: ComplexWorklet
   :file: GuideExampleCellEdgesFaces.cxx
   :caption: A more complex worklet.

We will discuss the many features available in the worklet framework throughout :partref:`part-advanced:Advanced Development`.
