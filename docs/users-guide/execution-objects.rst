==============================
Execution Objects
==============================

.. index::
   single: execution object
   single: worklet; execution object
   single: control signature; execution object

Although passing whole arrays and cell sets into a worklet is a convenient way to provide data to a worklet that is not divided by the input or output domain, they are sometimes not the best structures to represent data.
Thus, all worklets support a another type of argument called an *execution object*, or exec object for short, that provides a user-defined object directly to each invocation of the worklet.
This is defined by an ``ExecObject`` tag in the ``ControlSignature``.

Later in this chapter in :secref:`execution-objects:Designing Execution Objects` we will see how to implement an execution object that is provided to each instance of a worklet.
However, before that we will explore some of the execution objects created by other builtin |VTKm| objects such as :class:`vtkm::cont::ArrayHandle`.
These objects are used internally by |VTKm| when implementing the functionality of other ``ControlSignature`` arguments.
They also are often used as building blocks when constructing your own execution objects.


----------------------------------------
Interfaces to the Execution Environment
----------------------------------------

One of the main functions of |VTKm| classes like :class:`vtkm::cont::ArrayHandle` and :class:`vtkm::cont::CellSet` is to allow data to be defined in the control environment and then be used in the execution environment.
When using these objects with filters, worklets, or algorithms, this transition is handled automatically.
However, it is also possible to invoke the transfer for a known device.

Each class may have its own functions for transferring data from control environment to execution environment.
They typically take a :class:`vtkm::cont::DeviceAdapterId` to specify the device and a :class:`vtkm::cont::Token` to define the time during which the data must remain valid.
These methods return an object that must be passed to the execution environment running on the same device to be used.
We will start by describing the :class:`vtkm::cont::ArrayHandle` object, which manages transferring basic arrays between environments.
Most other execution objects are built from :class:`vtkm::cont::ArrayHandle` objects.

The :class:`vtkm::cont::ArrayHandle` class manages the transition from control to execution with a set of three methods that allocate, transfer, and ready the data in one operation.
These methods all start with the prefix ``Prepare`` and are meant to be called before some operation happens in the execution environment.
The methods are as follows.

* :func:`vtkm::cont::ArrayHandle::PrepareForInput`
  Copies data from the control to the execution environment, if necessary, and readies the data for read-only access.
* :func:`vtkm::cont::ArrayHandle::PrepareForInPlace`
  Copies the data from the control to the execution environment, if necessary, and readies the data for both reading and writing.
* :func:`vtkm::cont::ArrayHandle::PrepareForOutput`
  Allocates space (the size of which is given as a parameter) in the execution environment, if necessary, and readies the space for writing.

The :func:`vtkm::cont::ArrayHandle::PrepareForInput` and :func:`vtkm::cont::ArrayHandle::PrepareForInPlace` methods each take two arguments.
The first argument is the device adapter tag where execution will take place (see :secref:`managing-devices:Device Adapter Tag` for more information on device adapter tags).
The second argument is a reference to a :class:`vtkm::cont::Token`, which scopes the returned array portal, as described in :secref:`execution-objects:Specifying Object Scope with Tokens`.
:func:`vtkm::cont::ArrayHandle::PrepareForOutput` takes three arguments: the size of the space to allocate, the device adapter tag, and a reference to a :class:`vtkm::cont::Token` object.

Each of these ``Prepare`` methods returns an array portal that can be used in the execution environment.
:func:`vtkm::cont::ArrayHandle::PrepareForInput` returns an object of type :type:`vtkm::cont::ArrayHandle::ReadPortalType` whereas ``PrepareForInPlace`` and ``PrepareForOutput`` each return an object of type :type:`vtkm::cont::ArrayHandle::WritePortalType`.

Although these ``Prepare`` methods are called in the control environment, the returned array portal can only be used in the execution environment.
Thus, the portal must be passed to an invocation of the execution environment.

Most of the time, the passing of :class:`vtkm::cont::ArrayHandle` data to the execution environment is handled automatically by |VTKm|.
The most common need to call one of these ``Prepare`` methods is to build execution objects, described :ref:`below <execution-objects:Designing Execution Objects>`.

The following example is a contrived example for preparing arrays for the execution environment.
It is contrived because it would be easier to create a worklet or transform array handle to have the same effect, and in those cases |VTKm| would take care of the transfers internally.
More realistic examples are given later.

.. load-example:: ExecutionPortals
   :file: GuideExampleArrayHandle.cxx
   :caption: Using an execution array portal from an :class:`vtkm::cont::ArrayHandle`.

Other classes have their own ``Prepare-`` algorithms to get an execution object for a particular device.
For example, all the subclasses of :class:`vtkm::cont::CellSet` have a function named ``PrepareForInput()`` (e.g., :func:`vtkm::cont::CellSetExplicit::PrepareForInput` and :func:`vtkm::cont::CellSetStructured::PrepareForInput`).
These take a :class:`vtkm::cont::DeviceAdapterId`, a pair of tags specifying the visit and incident topology, and a :class:`vtkm::cont::Token`.
The returned object is the same connectivity object described in :secref:`globals:Whole Cell Sets`.


----------------------------------------
Specifying Object Scope with Tokens
----------------------------------------

One of the problems with receiving execution objects from other managed objects is that it is difficult to ensure that returned execution object remains valid.
For example, if you were to use :func:`vtkm::cont::ArrayHandle::PrepareForInput` to get an array portal for a :class:`vtkm::cont::ArrayHandle`, that array portal would become invalid if the array were freed.
If some code were to use that array portal, it would result in undefined behavior.

To prevent something like this from occurring, |VTKm| uses an object called :class:`vtkm::cont::Token`.
A :class:`vtkm::cont::Token` is a simple non-copyable object that gets attached to other |VTKm| objects such as :class:`vtkm::cont::ArrayHandle`.
While the :class:`vtkm::cont::Token` is attached, certain operations on the target object will block.

.. doxygenclass:: vtkm::cont::Token

As described in :secref:`execution-objects:Interfaces to the Execution Environment`, whenever an execution object is created, a :class:`vtkm::cont::Token` object must be provided.
That :class:`vtkm::cont::Token` is attached to the source object.
While it is attached, the source object prevents any changes that could invalidate the execution object.
For example, when a :class:`vtkm::cont::Token` is used to create an array portal, while the given token object exists, the returned portal is guaranteed to be valid and any conflicting operations on the :class:`vtkm::cont::ArrayHandle` will block.
Once the :class:`vtkm::cont::Token` is destroyed, the associated array portal may become invalid.
It is best to structure code such that the token and the execution object are in the same scope.

.. load-example:: ArrayPortalToken
   :file: GuideExampleArrayHandle.cxx
   :caption: Using a :class:`vtkm::cont::Token` to lock a :class:`vtkm::cont::ArrayHandle` while a portal is accessing it.

A :class:`vtkm::cont::Token` typically releases objects when it is destroyed by going out of scope.
If there is a reason to detach a token before it is destroyed, this can be done with the :func:`vtkm::cont::Token::DetachFromAll` method.

.. doxygenfunction:: vtkm::cont::Token::DetachFromAll

.. didyouknow::
   When a token is destroyed or detached, it does not immediately invalidate the execution objects it is associated with.
   This is both good and bad.
   It is good in that it simplifies code that is not managing objects on multiple threads so that scopes do not have to be continually created and destroyed.
   However, it is bad in that there is no automatic check that an object is being protected by a token.
   The code might appear to be working but then fail under different circumstances.
   Thus, be careful about using objects in multithreaded environments.

.. commonerrors::
   A :class:`vtkm::cont::Token` adds safety to prevent an object from being invalidated while it is still being used.
   However, a :class:`vtkm::cont::Token` will cause other code to block if necessary.
   This creates the possibility of deadlock, which can happen even in a single thread.
   Thus, a :class:`vtkm::cont::Token` should live just as long as needed and no more.


------------------------------
Designing Execution Objects
------------------------------

.. index::
   single: worklet; execution object
   single: control signature; execution object

It is possible to create your own execution objects.
These objects can be passed to a worklet using an :class:`ExecObject` tag in the ``ControlEnvironment``.
|VTKm| makes it straightforward to create your own execution objects.
These execution objects will have a management object in the control environment and then will create an execution object for a particular device.

The execution object you create must be a subclass of :class:`vtkm::cont::ExecutionObjectBase`.

.. doxygenstruct:: vtkm::cont::ExecutionObjectBase
   :members:

Your execution object must implement a ``PrepareForExecution()`` method declared with ``VTKM_CONT``.
``PrepareForExecution`` should take two arguments.
The first argument is the device adapter tag (usually a :class:`vtkm::cont::DeviceAdapterId`).
The second argument is a :class:`vtkm::cont::Token` object that should be used to scope any execution objects created internally.

The ``PrepareForExecution`` function creates an execution object that can be passed from the control environment to the execution environment and be usable in the execution environment.
Any method of the produced object used within the worklet must be declared with ``VTKM_EXEC`` or ``VTKM_EXEC_CONT``.

An execution object can refer to an array, but the array reference must be through an array portal for the execution environment.
This can be retrieved from the :func:`vtkm::cont::ArrayHandle::PrepareForInput` method as described in :secref:`execution-objects:Interfaces to the Execution Environment`.
Other |VTKm| data objects, such as the subclasses of :class:`vtkm::cont::CellSet`, have similar methods.

Returning to the example we have in :secref:`globals:Whole Arrays`, we are computing triangle quality quickly by looking up a value in a table.
In :numref:`ex:TriangleQualityWholeArray`, the table is passed directly to the worklet as a whole array.
However, there is some additional code involved to get the appropriate index into the table for a given triangle.
Let us say that we want to have the ability to compute triangle quality in many different worklets.
Rather than pass in a raw array, it would be better to encapsulate the functionality in an object.

We can do that by creating an execution object with a ``PrepareForExecution()`` method that creates an object that has the table stored inside and methods to compute the triangle quality.
The following example uses the table built in :numref:`ex:TriangleQualityWholeArray` to create such an object.

.. load-example:: TriangleQualityExecObject
   :file: GuideExampleTriangleQuality.cxx
   :caption: Using ``ExecObject`` to access a lookup table in a worklet.
