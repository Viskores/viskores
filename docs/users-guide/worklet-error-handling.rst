==============================
Worklet Error Handling
==============================

.. index::
   double: worklet; error handling
   double: execution environment; errors

It is sometimes the case during the execution of an algorithm that an error condition can occur that causes the computation to become invalid.
At such a time, it is important to raise an error to alert the calling code of the problem.
Since |VTKm| uses an exception mechanism to raise errors, we want an error in the execution environment to throw an exception.

However, throwing exceptions in a parallel algorithm is problematic.
Some accelerator architectures, like CUDA, do not even support throwing exceptions.
Even on architectures that do support exceptions, throwing them in a thread block can cause problems.
An exception raised in one thread may or may not be thrown in another, which increases the potential for deadlocks, and it is unclear how uncaught exceptions progress through thread blocks.

|VTKm| handles this problem by using a flag and check mechanism.
When a worklet (or other subclass of :class:`vtkm::exec::FunctorBase`) encounters an error, it can call its :func:`vtkm::exec::FunctorBase::RaiseError` method to flag the problem and record a message for the error.
Once all the threads terminate, the scheduler checks for the error, and if one exists it throws a \vtkmcont{ErrorExecution} exception in the control environment.
Thus, calling :func:`vtkm::exec::FunctorBase::RaiseError` looks like an exception was thrown from the perspective of the control environment code that invoked it.

.. load-example:: ExecutionErrors
   :file: GuideExampleErrorHandling.cxx
   :caption: Raising an error in the execution environment.

It is also worth noting that the :c:macro:`VTKM_ASSERT` macro described in :secref:`error-handling:Asserting Conditions` also works within worklets and other code running in the execution environment.
Of course, a failed assert will terminate execution rather than just raise an error so is best for checking invalid conditions for debugging purposes.
