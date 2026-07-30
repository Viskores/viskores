==============================
Try Execute
==============================

.. _chap:TryExecute:

.. index::
   single: try execute
   single: device adapter; try execute

Most operations in |Viskores| do not require specifying on which device to run.
For example, when using :class:`viskores::cont::Invoker` to execute a worklet,
you do not need to specify a device; it chooses a device for you.
Internally, :class:`viskores::cont::Invoker` has a mechanism to automatically
select a device, try it, and fall back to other devices if the first one fails.
We saw this at work in the implementation of filters in
:secref:`simple-worklets:Invoking a Worklet`.
Furthermore, :secref:`managing-devices:Specifying Devices` describes how users
can control which devices get used.

:class:`viskores::cont::Invoker` internally uses
:func:`viskores::cont::TryExecute` to choose a device.
:func:`viskores::cont::TryExecute` can also be used in other situations where
a specific device needs to be chosen.
It provides a simple, generic mechanism to run an algorithm that requires a
device adapter without directly specifying one.

.. doxygenfunction:: viskores::cont::TryExecute

To demonstrate the operation of :func:`viskores::cont::TryExecute`, consider
an operation to find the average value of an array.
Doing so with a given device adapter is a straightforward use of the reduction
operator.

.. load-example:: ArrayAverageImpl
   :file: GuideExampleTryExecute.cxx
   :caption: A function to find the average value of an array in parallel.

The function in :numref:`ex:ArrayAverageImpl` requires a device adapter.
(This is a somewhat contrived example as :func:`viskores::cont::Algorithm::Reduce`
will internally select a device if not provided one, but this is a way to have
:class:`viskores::cont::Timer` only synchronize the device being run on.)
We want to make an alternate version of this function that does not need a
specific device adapter but rather finds one to use.
To do this, we first make a functor as described in the
:func:`viskores::cont::TryExecute` API documentation.
It takes a device adapter tag as an argument, calls the version of the function
shown in :numref:`ex:ArrayAverageImpl`, and returns ``true`` when the operation
succeeds.
We then create a new version of the array average function that does not need a
specific device adapter tag and calls :func:`viskores::cont::TryExecute` with
the aforementioned functor.

.. load-example:: ArrayAverageTryExecute
   :file: GuideExampleTryExecute.cxx
   :caption: Using :func:`viskores::cont::TryExecute`.
