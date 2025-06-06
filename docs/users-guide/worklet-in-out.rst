==============================
Worklet Input Output Semantics
==============================

.. index::
   single: worklet; creating

The default scheduling of a worklet provides a 1 to 1 mapping from the input domain to the output domain.
For example, a :class:`viskores::worklet::WorkletMapField` gets run once for every item of the input array and produces one item for the output array.
Likewise, :class:`viskores::worklet::WorkletVisitCellsWithPoints` gets run once for every cell in the input topology and produces one associated item for the output field.

However, there are many operations that do not fall well into this 1 to 1 mapping procedure.
The operation might need to pass over elements that produce no value or the operation might need to produce multiple values for a single input element.
Such non 1 to 1 mappings can be achieved by defining a scatter or a mask (or both) on a worklet.

------------------------------
Scatter
------------------------------

.. index::
   double: worklet; scatter

A *scatter* allows you to specify for each input element how many output elements should be created.
For example, a scatter allows you to create two output elements for every input element.
A scatter could also allow you to drop every other input element from the output.
The following types of scatter are provided by |Viskores|.

.. doxygenstruct:: viskores::worklet::ScatterIdentity
   :members:
.. doxygenstruct:: viskores::worklet::ScatterUniform
   :members:
.. doxygenstruct:: viskores::worklet::ScatterCounting
   :members:
.. doxygenstruct:: viskores::worklet::ScatterPermutation
   :members:

.. didyouknow::
   Scatters are often used to create multiple outputs for a single input, but they can also be used to remove inputs from the output.
   In particular, if you provide a count of 0 in a :class:`viskores::worklet::ScatterCounting` count array, no outputs will be created for the associated input.
   To simply mask out some elements from the input, provide :class:`viskores::worklet::ScatterCounting` with a stencil array of 0's and 1's with a 0 for every element you want to remove and a 1 for every element you want to pass.
   You can also mix 0's with counts larger than 1 to drop some elements and add multiple results for other elements.
   :class:`viskores::worklet::ScatterPermutation` can similarly be used to remove input values by leaving them out of the permutation.

To define a scatter procedure, the worklet must provide a type definition named ``ScatterType``.
The ``ScatterType`` must be set to one of the aforementioned ``Scatter*`` classes.

.. load-example:: DeclareScatter
   :file: GuideExampleScatterCounting.cxx
   :caption: Declaration of a scatter type in a worklet.

.. index:: VisitIndex

When using a scatter that produces multiple outputs for a single input, the worklet is invoked multiple times with the same input values.
In such an event the worklet operator needs to distinguish these calls to produce the correct associated output.
This is done by declaring one of the ``ExecutionSignature`` arguments as ``VisitIndex``.
This tag will pass a :type:`viskores::IdComponent` to the worklet that identifies which invocation is being called.

.. index::
   single: input index
   single: output index

It is also the case that the when a scatter can produce multiple outputs for some input that the index of the input element is not the same as the ``WorkIndex``.
If the index to the input element is needed, you can use the ``InputIndex`` tag in the ``ExecutionSignature``.
It is also good practice to use the ``OutputIndex`` tag if the index to the output element is needed.

Most ``Scatter`` objects have a state, and this state must be passed to the :class:`viskores::cont::Invoker` when invoking the worklet.
In this case, the ``Scatter`` object should be passed as the second object to the call to the :class:`viskores::cont::Invoker` (after the worklet object).

.. load-example:: ConstructScatterForInvoke
   :file: GuideExampleScatterCounting.cxx
   :caption: Invoking with a custom scatter.

.. didyouknow::
   A scatter object does not have to be tied to a single worklet/invoker instance.
   In some cases it makes sense to use the same scatter object multiple times for worklets that have the same input to output mapping.
   Although this is not common, it can save time by reusing the set up computations of :class:`viskores::worklet::ScatterCounting`.

To demonstrate using scatters with worklets, we provide some contrived but illustrative examples.
The first example is a worklet that takes a pair of input arrays and interleaves them so that the first, third, fifth, and so on entries come from the first array and the second, fourth, sixth, and so on entries come from the second array.
We achieve this by using a :class:`viskores::worklet::ScatterUniform` of size 2 and using the ``VisitIndex`` to determine from which array to pull a value.

.. load-example:: ScatterUniform
   :file: GuideExampleScatterUniform.cxx
   :caption: Using :class:`viskores::worklet::ScatterUniform`.

The second example takes a collection of point coordinates and clips them by an axis-aligned bounding box.
It does this using a :class:`viskores::worklet::ScatterCounting` with an array containing 0 for all points outside the bounds and 1 for all points inside the bounds.
As is typical with this type of operation, we use another worklet with a default identity scatter to build the count array.

.. load-example:: ScatterCounting
   :file: GuideExampleScatterCounting.cxx
   :caption: Using :class:`viskores::worklet::ScatterCounting`.

The third example takes an input array and reverses the ordering.
It does this using a :class:`viskores::worklet::ScatterPermutation` with a permutation array generated from a :class:`viskores::cont::ArrayHandleCounting` counting down from the input array size to 0.

.. load-example:: ScatterPermutation
   :file: GuideExampleScatterPermutation.cxx
   :caption: Using :class:`viskores::worklet::ScatterPermutation`.

.. didyouknow::
   A :class:`viskores::worklet::ScatterPermutation` can have less memory usage than a :class:`viskores::worklet::ScatterCounting` when zeroing indices.
   By default, a :class:`viskores::worklet::ScatterPermutation` will omit all fields that are not specified in the input permutation, whereas :class:`viskores::worklet::ScatterCounting` requires 0 values.
   If mapping an input to an output that omits fields, consider using a :class:`viskores::worklet::ScatterPermutation` to save memory.

.. commonerrors::
   A permutation array provided to :class:`viskores::worklet::ScatterPermutation` can be filled with arbitrary id values.
   If an input permutation id exceeds the bounds of an input provided to a worklet, an out of bounds error will occur in the worklet functor.
   To prevent this kind of error, you should ensure that ids in the :class:`viskores::worklet::ScatterPermutation` do not exceed the bounds of provided inputs.


------------------------------
Mask
------------------------------

.. index::
   double: worklet; mask

A *mask* allows you to mask out particular output entries.
For example, a mask allows you to write out to array indices with an even index while leaving those with an odd index untouched.
A mask could also allow you to change values that match a certain criteria.
The worklet is only run for the indices for which a value is generated.
Because a worklet with a mask only writes to select indices, it is best to write to arrays with in/out semantics.

The following types of mask are provided by |Viskores|.

.. doxygenstruct:: viskores::worklet::MaskNone
   :members:
.. doxygenclass:: viskores::worklet::MaskIndices
   :members:
.. doxygenclass:: viskores::worklet::MaskSelect
   :members:

The constructor of :class:`viskores::worklet::MaskSelect` takes a :class:`viskores::cont::UnknownArrayHandle` and is precompiled for a set of expected array types.
However, if you have a custom array handle type like many of those in :chapref:`fancy-array-handles:Fancy Array Handles`, it is often more efficient to use :class:`viskores::worklet::MaskSelectTemplate`, which has a templated constructor to compile for a specific array handle type.

.. todo:: Add chapref to \ref{chap:Storage} when available.

.. doxygenclass:: viskores::worklet::MaskSelectTemplate

To define a mask procedure, the worklet must provide a type definition named ``MaskType``.
The ``MaskType`` must be set to one of the aforementioned ``Mask*`` classes.

.. load-example:: DeclareMask
   :file: GuideExampleMaskSelect.cxx
   :caption: Declaration of a mask type in a worklet.

When using a mask, there is no longer a 1-to-1 correspondence between the ``WorkIndex`` and the indices of the input and output.
If the index to the input or output element is needed, you can use the ``InputIndex`` tag and ``OutputIndex`` tag, respectively, in the ``ExecutionSignature``.

Most ``Mask`` objects have a state, and this state must be passed to the :class:`viskores::cont::Invoker` when invoking the worklet.
In this case, the ``Mask`` object should be passed as the second object to the call to the :class:`viskores::cont::Invoker` (after the worklet object).

.. load-example:: ConstructMaskForInvoke
   :file: GuideExampleMaskSelect.cxx
   :caption: Invoking with a custom mask.

.. didyouknow::
   A mask object does not have to be tied to a single worklet/invoker instance.
   In some cases it makes sense to use the same mask object multiple times for worklets that have the same input to output mapping.
   Although this is not common, it can save time by reusing the set up computations of :class:`viskores::worklet::MaskSelect`.

To demonstrate using a mask with a worklet, here is a contrived but illustrative example.
In this example, we write a worklet that takes an array and for each entry replaces the value with the number in the Fibonacci sequence closest to that value.

.. load-example:: MaskSelectWorklet
   :file: GuideExampleMaskSelect.cxx
   :caption: A worklet for finding the closest Fibonacci value.

A simple observation is that the Fibonacci sequence contains all positive integers up to 3.
Thus, values for these small positive numbers, the value will not change.
If it is likely that there will be many such small values, we can potentially speed up our operation by only operating on values greater than 3.

.. load-example:: MaskSelectRun
   :file: GuideExampleMaskSelect.cxx
   :caption: Using a mask to select which array entries to update.

.. didyouknow::
   In :numref:`ex:MaskSelectRun` a :class:`viskores::cont::ArrayHandleTransform` is used to create a select array of 0's and 1's in place.
   To accelerate the construction of the mask indices, a :class:`viskores::worklet::MaskSelectTemplate` is used.
   This is trivial subclass of :class:`viskores::worklet::MaskSelect` so it can be constructed and then used for the mask of the worklet.
