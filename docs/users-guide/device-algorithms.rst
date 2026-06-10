==============================
Device Algorithms
==============================

.. index::
   single: device adapter; algorithm
   single: algorithm

As described in :chapref:`general-approach:General Approach`, |Viskores| is built around the concept of a :index:`device adapter` that encapsulates the necessary features of each device on which |Viskores| can run.
At the core of the device adapter is a collection of basic algorithms optimized for the specific device.
Many features of |Viskores|, such as worklets, are built on top of these device algorithms.
Using these higher-level structures simplifies programming.

However, it is sometimes desirable to run these algorithms directly.
|Viskores| comes with the :struct:`viskores::cont::Algorithm` class that provides a set of algorithms that can be invoked in the control environment and are run in the execution environment.
All algorithms also accept an optional device adapter argument.

.. doxygenstruct:: viskores::cont::Algorithm

The following sections document the available methods.

.. didyouknow::
   Many of the following device adapter algorithms take input and output :class:`viskores::cont::ArrayHandle` objects, and these functions will handle their own memory management.
   This means that it is unnecessary to allocate output arrays.
   For example, it is unnecessary to call :func:`viskores::cont::ArrayHandle::Allocate` for the output array passed to the :func:`viskores::cont::Algorithm::Copy` method.


------------------------------
BitFieldToUnorderedSet
------------------------------

.. index::
   single: bit field to unordered set
   single: algorithm; bit field to unordered set
   single: device adapter; algorithm; bit field to unordered set

.. doxygenfunction:: viskores::cont::Algorithm::BitFieldToUnorderedSet(viskores::cont::DeviceAdapterId devId, const viskores::cont::BitField& bits, viskores::cont::ArrayHandle<Id, IndicesStorage>& indices)
.. doxygenfunction:: viskores::cont::Algorithm::BitFieldToUnorderedSet(const viskores::cont::BitField &bits, viskores::cont::ArrayHandle<Id, IndicesStorage> &indices)

The :func:`viskores::cont::Algorithm::BitFieldToUnorderedSet` method creates a unique, unsorted list of indices denoting which bits are set in a bit field.
For example, running :func:`viskores::cont::Algorithm::BitFieldToUnorderedSet` on an input of ``[0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1]`` would return an array containing ``[2, 4, 6, 7, 9, 10, 11]`` or those numbers in some other order.

.. load-example:: DeviceAdapterAlgorithmBitFieldToUnorderedSet
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``BitFieldToUnorderedSet`` algorithm.


------------------------------
Copy
------------------------------

.. index::
   single: copy
   single: algorithm; copy
   single: device adapter; algorithm; copy

.. doxygenfunction:: viskores::cont::Algorithm::Copy(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<U, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::Copy(const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<U, COut>& output)

The :func:`viskores::cont::Algorithm::Copy` method copies data from an input array to an output array.
The copy takes place in the execution environment.

.. load-example:: DeviceAdapterAlgorithmCopy
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``Copy`` algorithm.


------------------------------
CopyIf
------------------------------

.. index::
   single: stream compact
   single: copy if
   single: algorithm; stream compact
   single: algorithm; copy if
   single: device adapter; algorithm; stream compact
   single: device adapter; algorithm; copy if

.. doxygenfunction:: viskores::cont::Algorithm::CopyIf(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<U, CStencil>& stencil, viskores::cont::ArrayHandle<T, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<U, CStencil>& stencil, viskores::cont::ArrayHandle<T, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::CopyIf(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<U, CStencil>& stencil, viskores::cont::ArrayHandle<T, COut>& output, UnaryPredicate unary_predicate)
.. doxygenfunction:: viskores::cont::Algorithm::CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<U, CStencil>& stencil, viskores::cont::ArrayHandle<T, COut>& output, UnaryPredicate unary_predicate)

The :func:`viskores::cont::Algorithm::CopyIf` method selectively removes values from an array.
The :index:`copy if` algorithm is also sometimes referred to as :index:`stream compact`.
The first argument, the input, is an :class:`viskores::cont::ArrayHandle` to be compacted by removing elements.
The second argument, the stencil, is an :class:`viskores::cont::ArrayHandle` of equal size with flags indicating whether the corresponding input value is to be copied to the output.
The third argument is an output :class:`viskores::cont::ArrayHandle` whose length is set to the number of true flags in the stencil, and the passed values are put in order in the output array.

:func:`viskores::cont::Algorithm::CopyIf` also accepts an optional fourth argument that is a unary predicate to determine what values in the stencil, the second argument, should be considered true.
See :secref:`device-algorithms:Predicates and Operators` for more information on unary predicates.
The unary predicate determines the true or false value of the stencil that determines whether a given entry is copied.
If no unary predicate is given, then :func:`viskores::cont::Algorithm::CopyIf` will copy all values whose stencil value is not equal to 0 or the closest equivalent to it.
More specifically, it copies values not equal to :func:`viskores::TypeTraits::ZeroInitialization`.

.. load-example:: DeviceAdapterAlgorithmCopyIf
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``CopyIf`` algorithm.


------------------------------
CopySubRange
------------------------------

.. index::
   single: copy sub range
   single: algorithm; copy sub range
   single: device adapter; algorithm; copy sub range

.. doxygenfunction:: viskores::cont::Algorithm::CopySubRange(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, viskores::Id inputStartIndex, viskores::Id numberOfElementsToCopy, viskores::cont::ArrayHandle<U, COut>& output, viskores::Id outputIndex)
.. doxygenfunction:: viskores::cont::Algorithm::CopySubRange(const viskores::cont::ArrayHandle<T, CIn>& input, viskores::Id inputStartIndex, viskores::Id numberOfElementsToCopy, viskores::cont::ArrayHandle<U, COut>& output, viskores::Id outputIndex)

The :func:`viskores::cont::Algorithm::CopySubRange` method copies the contents of a section of one :class:`viskores::cont::ArrayHandle` to another.
The first argument is the input :class:`viskores::cont::ArrayHandle`.
The second argument is the index from which to start copying data.
The third argument is the number of values to copy from the input to the output.
The fourth argument is the output :class:`viskores::cont::ArrayHandle`, which will be grown if it is not large enough.
The fifth argument, which is optional, is the index in the output array to start copying data to.
If the output index is not specified, data are copied to the beginning of the output array.

.. load-example:: DeviceAdapterAlgorithmCopySubRange
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``CopySubRange`` algorithm.


------------------------------
CountSetBits
------------------------------

.. index::
   single: count set bits
   single: algorithm; count set bits
   single: device adapter; algorithm; count set bits

.. doxygenfunction:: viskores::cont::Algorithm::CountSetBits(viskores::cont::DeviceAdapterId devId, const viskores::cont::BitField& bits)
.. doxygenfunction:: viskores::cont::Algorithm::CountSetBits(const viskores::cont::BitField& bits)

The :func:`viskores::cont::Algorithm::CountSetBits` method returns the total number of set bits in a :class:`viskores::cont::BitField`.
For example, running :func:`viskores::cont::Algorithm::CountSetBits` on an input of ``[0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1]`` would return 7.

.. load-example:: DeviceAdapterAlgorithmCountSetBits
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``CountSetBits`` algorithm.


------------------------------
Fill
------------------------------

.. index::
   single: fill
   single: algorithm; fill
   single: device adapter; algorithm; fill

.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::DeviceAdapterId devId, viskores::cont::BitField& bits, bool value, viskores::Id numBits)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::BitField& bits, bool value, viskores::Id numBits)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::DeviceAdapterId devId, viskores::cont::BitField& bits, bool value)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::BitField& bits, bool value)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::DeviceAdapterId devId, viskores::cont::BitField& bits, WordType word, viskores::Id numBits)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::BitField& bits, WordType word, viskores::Id numBits)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::DeviceAdapterId devId, viskores::cont::BitField& bits, WordType word)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::BitField& bits, WordType word)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::DeviceAdapterId devId, viskores::cont::ArrayHandle<T, S>& handle, const T& value)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::ArrayHandle<T, S>& handle, const T& value)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::DeviceAdapterId devId, viskores::cont::ArrayHandle<T, S>& handle, const T& value, const viskores::Id numValues)
.. doxygenfunction:: viskores::cont::Algorithm::Fill(viskores::cont::ArrayHandle<T, S>& handle, const T& value, const viskores::Id numValues)

The :func:`viskores::cont::Algorithm::Fill` methods fill a :class:`viskores::cont::BitField` or :class:`viskores::cont::ArrayHandle` with a specific pattern of bits or values.
For a :class:`viskores::cont::BitField`, it is possible to supply a boolean value or a ``WordType``.
For boolean values, all bits are set to 1 if the value is true, or 0 if the value is false.
For word masks, the ``WordType`` must be an unsigned integral type; this value is stamped across the :class:`viskores::cont::BitField`.
For an :class:`viskores::cont::ArrayHandle`, the entire array is filled with the provided value.
For both types, if a ``numValues`` argument is provided the array is resized appropriately and filled with the given value.

.. load-example:: DeviceAdapterAlgorithmFill
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``Fill`` algorithm.


------------------------------
LowerBounds
------------------------------

.. index::
   single: lower bounds
   single: algorithm; lower bounds
   single: device adapter; algorithm; lower bounds

.. doxygenfunction:: viskores::cont::Algorithm::LowerBounds(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<T, CVal>& values, viskores::cont::ArrayHandle<viskores::Id, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::LowerBounds(const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<T, CVal>& values, viskores::cont::ArrayHandle<viskores::Id, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::LowerBounds(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<T, CVal>& values, viskores::cont::ArrayHandle<viskores::Id, COut>& output, BinaryCompare binary_compare)
.. doxygenfunction:: viskores::cont::Algorithm::LowerBounds(const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<T, CVal>& values, viskores::cont::ArrayHandle<viskores::Id, COut>& output, BinaryCompare binary_compare)
.. doxygenfunction:: viskores::cont::Algorithm::LowerBounds(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<viskores::Id, CIn>& input, viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
.. doxygenfunction:: viskores::cont::Algorithm::LowerBounds(const viskores::cont::ArrayHandle<viskores::Id, CIn>& input, viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)

The :func:`viskores::cont::Algorithm::LowerBounds` method takes three arguments.
The first argument is an :class:`viskores::cont::ArrayHandle` of sorted values.
The second argument is another :class:`viskores::cont::ArrayHandle` of items to find in the first array.
:func:`viskores::cont::Algorithm::LowerBounds` finds the index of the first item that is greater than or equal to the target value, much like the ``std::lower_bound`` STL algorithm.
The results are returned in an :class:`viskores::cont::ArrayHandle` given in the third argument.

There are two specializations of :func:`viskores::cont::Algorithm::LowerBounds`.
The first takes an additional comparison function that defines the less-than operation.
The second specialization takes only two parameters.
The first is an :class:`viskores::cont::ArrayHandle` of sorted :type:`viskores::Id` values and the second is an :class:`viskores::cont::ArrayHandle` of :type:`viskores::Id` values to find in the first list.
The results are written back out to the second array.
This second specialization is useful for inverting index maps.

.. load-example:: DeviceAdapterAlgorithmLowerBounds
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``LowerBounds`` algorithm.


------------------------------
Reduce
------------------------------

.. index::
   single: reduce
   single: algorithm; reduce
   single: device adapter; algorithm; reduce

.. doxygenfunction:: viskores::cont::Algorithm::Reduce(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue)
.. doxygenfunction:: viskores::cont::Algorithm::Reduce(const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue)
.. doxygenfunction:: viskores::cont::Algorithm::Reduce(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue, BinaryFunctor binary_functor)
.. doxygenfunction:: viskores::cont::Algorithm::Reduce(const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue, BinaryFunctor binary_functor)

The :func:`viskores::cont::Algorithm::Reduce` method takes an input array, initial value, and binary function and computes a "total" of applying the binary function to all entries in the array.
The provided binary function must be associative, but it need not be commutative.
There is a specialization of :func:`viskores::cont::Algorithm::Reduce` that does not take a binary function and computes the sum.

.. load-example:: DeviceAdapterAlgorithmReduce
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``Reduce`` algorithm.

.. commonerrors::
   When using the :func:`viskores::cont::Algorithm::Reduce` method, it is important to match the types of the initial value, array values, and the arguments of the reduction operator.
   For example, in :exlineref:`DeviceAdapterAlgorithmReduce:reduce-literal` notice that the initial value, the literal ``0``, is specifically cast as a :type:`viskores::Id` (i.e., ``viskores::Id{ 0 }``).
   This is because the default reduction operator expects the same type for both its input arguments and output argument.
   Thus the type initial value should match that of the types in the array being reduced.
   The literal value ``0`` is likely to be interpreted by the compiler as an ``int``, which may not be the exact same type as :type:`viskores::Id`.
   Thus, an explicit type is appropriate here.


------------------------------
ReduceByKey
------------------------------

.. index::
   single: reduce by key
   single: algorithm; reduce by key
   single: device adapter; algorithm; reduce by key

.. doxygenfunction:: viskores::cont::Algorithm::ReduceByKey(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CKeyIn>& keys, const viskores::cont::ArrayHandle<U, CValIn>& values, viskores::cont::ArrayHandle<T, CKeyOut>& keys_output, viskores::cont::ArrayHandle<U, CValOut>& values_output, BinaryFunctor binary_functor)
.. doxygenfunction:: viskores::cont::Algorithm::ReduceByKey(const viskores::cont::ArrayHandle<T, CKeyIn>& keys, const viskores::cont::ArrayHandle<U, CValIn>& values, viskores::cont::ArrayHandle<T, CKeyOut>& keys_output, viskores::cont::ArrayHandle<U, CValOut>& values_output, BinaryFunctor binary_functor)

The :func:`viskores::cont::Algorithm::ReduceByKey` method works similarly to the :func:`viskores::cont::Algorithm::Reduce` method except that it takes an additional array of keys, which must be the same length as the values being reduced.
The arrays are partitioned into segments that have identical adjacent keys, and a separate reduction is performed on each partition.
The unique keys and reduced values are returned in separate arrays.

.. load-example:: DeviceAdapterAlgorithmReduceByKey
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``ReduceByKey`` algorithm.


------------------------------
ScanInclusive
------------------------------

.. index::
   single: scan; inclusive
   single: algorithm; scan; inclusive
   single: device adapter; algorithm; scan; inclusive

.. doxygenfunction:: viskores::cont::Algorithm::ScanInclusive(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::ScanInclusive(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output, BinaryFunctor binary_functor)
.. doxygenfunction:: viskores::cont::Algorithm::ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output, BinaryFunctor binary_functor)

The :func:`viskores::cont::Algorithm::ScanInclusive` method takes an input and an output :class:`viskores::cont::ArrayHandle` and performs a running sum on the input array.
For inclusive scans, the running sum value for position :math:`i` in the input array *includes* the element at position :math:`i`.
The first value in the output is the same as the first value in the input.
The second value in the output is the sum of the first two values in the input.
The third value in the output is the sum of the first three values of the input, and so on.
If the input and output array are the same, then the operation is done in place.
:func:`viskores::cont::Algorithm::ScanInclusive` returns the sum of all values in the input.
There are two forms of :func:`viskores::cont::Algorithm::ScanInclusive`: one performs the sum using addition whereas the other accepts a custom binary function to use as the sum operator.

.. load-example:: DeviceAdapterAlgorithmScanInclusive
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``ScanInclusive`` algorithm.


------------------------------
ScanInclusiveByKey
------------------------------

.. index::
   single: scan; inclusive by key
   single: algorithm; scan; inclusive by key
   single: device adapter; algorithm; scan; inclusive by key

.. doxygenfunction:: viskores::cont::Algorithm::ScanInclusiveByKey(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, KIn>& keys, const viskores::cont::ArrayHandle<U, VIn>& values, viskores::cont::ArrayHandle<U, VOut>& values_output, BinaryFunctor binary_functor)
.. doxygenfunction:: viskores::cont::Algorithm::ScanInclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys, const viskores::cont::ArrayHandle<U, VIn>& values, viskores::cont::ArrayHandle<U, VOut>& values_output, BinaryFunctor binary_functor)
.. doxygenfunction:: viskores::cont::Algorithm::ScanInclusiveByKey(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, KIn>& keys, const viskores::cont::ArrayHandle<U, VIn>& values, viskores::cont::ArrayHandle<U, VOut>& values_output)
.. doxygenfunction:: viskores::cont::Algorithm::ScanInclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys, const viskores::cont::ArrayHandle<U, VIn>& values, viskores::cont::ArrayHandle<U, VOut>& values_output)

The :func:`viskores::cont::Algorithm::ScanInclusiveByKey` method works similarly to the :func:`viskores::cont::Algorithm::ScanInclusive` method except that it takes an additional array of keys, which must be the same length as the values being scanned.
The arrays are partitioned into segments that have identical adjacent keys, and a separate scan is performed on each partition.
Only the scanned values are returned.

.. load-example:: DeviceAdapterAlgorithmScanInclusiveByKey
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``ScanInclusiveByKey`` algorithm.


------------------------------
ScanExclusive
------------------------------

.. index::
   single: scan; exclusive
   single: algorithm; scan; exclusive
   single: device adapter; algorithm; scan; exclusive

.. doxygenfunction:: viskores::cont::Algorithm::ScanExclusive(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::ScanExclusive(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output, BinaryFunctor binaryFunctor, const T& initialValue)
.. doxygenfunction:: viskores::cont::Algorithm::ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output, BinaryFunctor binaryFunctor, const T& initialValue)

The :func:`viskores::cont::Algorithm::ScanExclusive` method takes an input and an output :class:`viskores::cont::ArrayHandle` and performs a running sum on the input array.
For exclusive scans, the running sum value for position :math:`i` in the input array *excludes* the element at position :math:`i`.
The first value in the output is always 0.
The second value in the output is the same as the first value in the input.
The third value in the output is the sum of the first two values in the input.
The fourth value in the output is the sum of the first three values of the input, and so on.
:func:`viskores::cont::Algorithm::ScanExclusive` returns the sum of all values in the input.
If the input and output array are the same, then the operation is done in place.
There are two forms of :func:`viskores::cont::Algorithm::ScanExclusive`.
The first performs the sum using addition.
The second form accepts a custom binary functor to use as the "sum" operator and a custom initial value instead of 0.

.. load-example:: DeviceAdapterAlgorithmScanExclusive
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``ScanExclusive`` algorithm.


------------------------------
ScanExclusiveByKey
------------------------------

.. index::
   single: scan; exclusive by key
   single: algorithm; scan; exclusive by key
   single: device adapter; algorithm; scan; exclusive by key

.. doxygenfunction:: viskores::cont::Algorithm::ScanExclusiveByKey(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, KIn>& keys, const viskores::cont::ArrayHandle<U, VIn>& values, viskores::cont::ArrayHandle<U, VOut>& output, const U& initialValue, BinaryFunctor binaryFunctor)
.. doxygenfunction:: viskores::cont::Algorithm::ScanExclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys, const viskores::cont::ArrayHandle<U, VIn>& values, viskores::cont::ArrayHandle<U, VOut>& output, const U& initialValue, BinaryFunctor binaryFunctor)
.. doxygenfunction:: viskores::cont::Algorithm::ScanExclusiveByKey(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, KIn>& keys, const viskores::cont::ArrayHandle<U, VIn>& values, viskores::cont::ArrayHandle<U, VOut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::ScanExclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys, const viskores::cont::ArrayHandle<U, VIn>& values, viskores::cont::ArrayHandle<U, VOut>& output)

The :func:`viskores::cont::Algorithm::ScanExclusiveByKey` method works similarly to the :func:`viskores::cont::Algorithm::ScanExclusive` method except that it takes an additional array of keys, which must be the same length as the values being scanned.
The arrays are partitioned into segments that have identical adjacent keys, and a separate scan is performed on each partition.
Only the scanned values are returned.

.. load-example:: DeviceAdapterAlgorithmScanExclusiveByKey
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``ScanExclusiveByKey`` algorithm.


------------------------------
ScanExtended
------------------------------

.. index::
   single: scan extend
   single: algorithm; scan extend
   single: device adapter; algorithm; scan extend

.. doxygenfunction:: viskores::cont::Algorithm::ScanExtended(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::ScanExtended(const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::ScanExtended(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output, BinaryFunctor binaryFunctor, const T& initialValue)
.. doxygenfunction:: viskores::cont::Algorithm::ScanExtended(const viskores::cont::ArrayHandle<T, CIn>& input, viskores::cont::ArrayHandle<T, COut>& output, BinaryFunctor binaryFunctor, const T& initialValue)

The :func:`viskores::cont::Algorithm::ScanExtended` computes an extended prefix sum operation on the input :class:`viskores::cont::ArrayHandle` and stores it in a provided output :class:`viskores::cont::ArrayHandle`.
The output array has length 1 greater than the input array.
:func:`viskores::cont::Algorithm::ScanExtended` is a combination of the :func:`viskores::cont::Algorithm::ScanInclusive` and :func:`viskores::cont::Algorithm::ScanExclusive` methods.
The exclusive scan values are stored in indices :math:`0` through :math:`size-1`.
The inclusive scan values are stored in indices :math:`1` through :math:`size`.
The first entry in the resulting array is 0, or the specified initial value, like with the exclusive scan.
The last entry in the resulting array is the sum total like with the inclusive scan.
Unlike the two referenced methods, :func:`viskores::cont::Algorithm::ScanExtended` does not return the total sum.
By using an :class:`viskores::cont::ArrayHandleView`, arrays containing both inclusive and exclusive scans can be generated from an extended scan with minimal memory usage by referencing the correct indices in the extended scan output.

This algorithm may be more efficient than :func:`viskores::cont::Algorithm::ScanInclusive` and :func:`viskores::cont::Algorithm::ScanExclusive` on some devices; this algorithm may be able to avoid copying the total sum to the control environment to return.
:func:`viskores::cont::Algorithm::ScanExtended` is similar to the STL partial sum function, with the exception that it does not perform a serial summation.
This means that if you have defined a custom plus operator for ``T`` it must be associative, or you will get inconsistent results.

The first form performs the sum using addition.
The second form accepts a custom binary functor to use as the operator and a custom initial value instead of 0.

.. load-example:: DeviceAdapterAlgorithmScanExtended
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``ScanExtended`` algorithm.

:func:`viskores::cont::Algorithm::ScanExtended` can be used to create a running sum that is quickly reversible.
If you subtract two consecutive values of a scan, you get back the original value.
This is convenient if you need both the input and output of a scan; you can throw away the input and use differences of the output.
However, :func:`viskores::cont::Algorithm::ScanInclusive` does not write out the initial value, so you cannot get back the original value at the beginning without a special condition.
Likewise, :func:`viskores::cont::Algorithm::ScanExclusive` does not write out the total sum, so you cannot get back the original value at the end without a special condition.
:func:`viskores::cont::Algorithm::ScanExtended` solves this problem by extending the array by 1.
The original value for index :math:`i` can be retrieved by subtracting the scan value at index :math:`i` from the value at index :math:`i+1` anywhere in the array, including at the beginning and end.
This is particularly useful for storing packed arrays in structures like :class:`viskores::cont::CellSetExplicit` and :class:`viskores::cont::ArrayHandleGroupVecVariable`.


------------------------------
Schedule
------------------------------

.. index::
   single: schedule
   single: algorithm; schedule
   single: device adapter; algorithm; schedule

The :func:`viskores::cont::Algorithm::Schedule` method takes a functor as its first argument and invokes it a number of times specified by the second argument.
It should be assumed that each invocation of the functor occurs on a separate thread, although in practice there could be some thread sharing.

There are two versions of the :func:`viskores::cont::Algorithm::Schedule` method.
The first version takes a :type:`viskores::Id` and invokes the functor that number of times.
The second version takes a :type:`viskores::Id3` and invokes the functor once for every entry in a 3D array of the given dimensions.

.. doxygenfunction:: viskores::cont::Algorithm::Schedule(viskores::cont::DeviceAdapterId devId, Functor functor, viskores::Id numInstances)
.. doxygenfunction:: viskores::cont::Algorithm::Schedule(viskores::cont::internal::HintList<Hints...> hints, Functor functor, viskores::Id numInstances)
.. doxygenfunction:: viskores::cont::Algorithm::Schedule(Functor functor, viskores::Id numInstances)
.. doxygenfunction:: viskores::cont::Algorithm::Schedule(viskores::cont::DeviceAdapterId devId, Functor functor, viskores::Id3 numInstances)
.. doxygenfunction:: viskores::cont::Algorithm::Schedule(viskores::cont::internal::HintList<Hints...> hints, Functor functor, viskores::Id3 numInstances)
.. doxygenfunction:: viskores::cont::Algorithm::Schedule(Functor functor, viskores::Id3 numInstances)

The functor is expected to be an object with a const overloaded parentheses operator.
The operator takes as a parameter the index of the invocation, which is either a :type:`viskores::Id` or a :type:`viskores::Id3` depending on what version of :func:`viskores::cont::Algorithm::Schedule` is being used.
The functor must also subclass :class:`viskores::exec::FunctorBase`, which provides the error handling facilities for the execution environment.
:class:`viskores::exec::FunctorBase` contains a public method named :func:`viskores::exec::FunctorBase::RaiseError` that takes a message and will cause a :class:`viskores::cont::ErrorExecution` exception to be thrown in the control environment.


------------------------------
Sort
------------------------------

.. index::
   single: sort
   single: algorithm; sort
   single: device adapter; algorithm; sort

.. doxygenfunction:: viskores::cont::Algorithm::Sort(viskores::cont::DeviceAdapterId devId, viskores::cont::ArrayHandle<T, Storage>& values)
.. doxygenfunction:: viskores::cont::Algorithm::Sort(viskores::cont::ArrayHandle<T, Storage>& values)
.. doxygenfunction:: viskores::cont::Algorithm::Sort(viskores::cont::DeviceAdapterId devId, viskores::cont::ArrayHandle<T, Storage>& values, BinaryCompare binary_compare)
.. doxygenfunction:: viskores::cont::Algorithm::Sort(viskores::cont::ArrayHandle<T, Storage>& values, BinaryCompare binary_compare)

The :func:`viskores::cont::Algorithm::Sort` method provides an unstable sort of an array.
There are two forms of the :func:`viskores::cont::Algorithm::Sort` method.
The first takes an :class:`viskores::cont::ArrayHandle` and sorts the values in place.
The second takes an additional argument that is a functor that provides the comparison operation for the sort.

.. load-example:: DeviceAdapterAlgorithmSort
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``Sort`` algorithm.


------------------------------
SortByKey
------------------------------

.. index::
   single: sort; by key
   single: algorithm; sort; by key
   single: device adapter; algorithm; sort; by key

.. doxygenfunction:: viskores::cont::Algorithm::SortByKey(viskores::cont::DeviceAdapterId devId, viskores::cont::ArrayHandle<T, StorageT>& keys, viskores::cont::ArrayHandle<U, StorageU>& values)
.. doxygenfunction:: viskores::cont::Algorithm::SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys, viskores::cont::ArrayHandle<U, StorageU>& values)
.. doxygenfunction:: viskores::cont::Algorithm::SortByKey(viskores::cont::DeviceAdapterId devId, viskores::cont::ArrayHandle<T, StorageT>& keys, viskores::cont::ArrayHandle<U, StorageU>& values, BinaryCompare binary_compare)
.. doxygenfunction:: viskores::cont::Algorithm::SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys, viskores::cont::ArrayHandle<U, StorageU>& values, BinaryCompare binary_compare)

The :func:`viskores::cont::Algorithm::SortByKey` method works similarly to the :func:`viskores::cont::Algorithm::Sort` method except that it takes two :class:`viskores::cont::ArrayHandle` objects: an array of keys and a corresponding array of values.
The sort orders the array of keys in ascending values and also reorders the values so they remain paired with the same key.
Like :func:`viskores::cont::Algorithm::Sort`, :func:`viskores::cont::Algorithm::SortByKey` has a version that sorts by the default less-than operator and a version that accepts a custom comparison functor.

.. load-example:: DeviceAdapterAlgorithmSortByKey
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``SortByKey`` algorithm.


------------------------------
Synchronize
------------------------------

.. index::
   single: synchronize
   single: algorithm; synchronize
   single: device adapter; algorithm; synchronize

The :func:`viskores::cont::Algorithm::Synchronize` method waits for any asynchronous operations running on the device to complete and then returns.

.. doxygenfunction:: viskores::cont::Algorithm::Synchronize(viskores::cont::DeviceAdapterId devId)
.. doxygenfunction:: viskores::cont::Algorithm::Synchronize()


------------------------------
Transform
------------------------------

.. index::
   single: transform
   single: algorithm; transform
   single: device adapter; algorithm; transform

.. doxygenfunction:: viskores::cont::Algorithm::Transform(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, StorageT>& input1, const viskores::cont::ArrayHandle<U, StorageU>& input2, viskores::cont::ArrayHandle<V, StorageV>& output, BinaryFunctor binaryFunctor)
.. doxygenfunction:: viskores::cont::Algorithm::Transform(const viskores::cont::ArrayHandle<T, StorageT>& input1, const viskores::cont::ArrayHandle<U, StorageU>& input2, viskores::cont::ArrayHandle<V, StorageV>& output, BinaryFunctor binaryFunctor)

The :func:`viskores::cont::Algorithm::Transform` method applies a given binary operation function element-wise on two input arrays, storing the result in a provided output array.
The number of elements in the input arrays do not have to be the same; the output array will have the same number of elements as the smaller of the two input arrays.

.. load-example:: DeviceAdapterAlgorithmTransform
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``Transform`` algorithm.


------------------------------
Unique
------------------------------

.. index::
   single: unique
   single: algorithm; unique
   single: device adapter; algorithm; unique

The :func:`viskores::cont::Algorithm::Unique` method removes all duplicate values in an :class:`viskores::cont::ArrayHandle`.
The method will only find duplicates if they are adjacent to each other in the array.
The easiest way to ensure that duplicate values are adjacent is to sort the array first.

There are two versions of :func:`viskores::cont::Algorithm::Unique`.
The first uses the equals operator to compare entries.
The second accepts a binary functor to perform the comparisons.

.. doxygenfunction:: viskores::cont::Algorithm::Unique(viskores::cont::DeviceAdapterId devId, viskores::cont::ArrayHandle<T, Storage>& values)
.. doxygenfunction:: viskores::cont::Algorithm::Unique(viskores::cont::ArrayHandle<T, Storage>& values)
.. doxygenfunction:: viskores::cont::Algorithm::Unique(viskores::cont::DeviceAdapterId devId, viskores::cont::ArrayHandle<T, Storage>& values, BinaryCompare binary_compare)
.. doxygenfunction:: viskores::cont::Algorithm::Unique(viskores::cont::ArrayHandle<T, Storage>& values, BinaryCompare binary_compare)

.. load-example:: DeviceAdapterAlgorithmUnique
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``Unique`` algorithm.


------------------------------
UpperBounds
------------------------------

.. index::
   single: upper bounds
   single: algorithm; upper bounds
   single: device adapter; algorithm; upper bounds

.. doxygenfunction:: viskores::cont::Algorithm::UpperBounds(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<T, CVal>& values, viskores::cont::ArrayHandle<viskores::Id, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::UpperBounds(const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<T, CVal>& values, viskores::cont::ArrayHandle<viskores::Id, COut>& output)
.. doxygenfunction:: viskores::cont::Algorithm::UpperBounds(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<T, CVal>& values, viskores::cont::ArrayHandle<viskores::Id, COut>& output, BinaryCompare binary_compare)
.. doxygenfunction:: viskores::cont::Algorithm::UpperBounds(const viskores::cont::ArrayHandle<T, CIn>& input, const viskores::cont::ArrayHandle<T, CVal>& values, viskores::cont::ArrayHandle<viskores::Id, COut>& output, BinaryCompare binary_compare)
.. doxygenfunction:: viskores::cont::Algorithm::UpperBounds(viskores::cont::DeviceAdapterId devId, const viskores::cont::ArrayHandle<viskores::Id, CIn>& input, viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
.. doxygenfunction:: viskores::cont::Algorithm::UpperBounds(const viskores::cont::ArrayHandle<viskores::Id, CIn>& input, viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)

The :func:`viskores::cont::Algorithm::UpperBounds` method takes three arguments.
The first argument is an :class:`viskores::cont::ArrayHandle` of sorted values.
The second argument is another :class:`viskores::cont::ArrayHandle` of items to find in the first array.
:func:`viskores::cont::Algorithm::UpperBounds` finds the index of the first item that is greater than the target value, much like the ``std::upper_bound`` STL algorithm.
The results are returned in an :class:`viskores::cont::ArrayHandle` given in the third argument.

There are two specializations of :func:`viskores::cont::Algorithm::UpperBounds`.
The first takes an additional comparison function that defines the less-than operation.
The second takes only two parameters.
The first is an :class:`viskores::cont::ArrayHandle` of sorted :type:`viskores::Id` values and the second is an :class:`viskores::cont::ArrayHandle` of :type:`viskores::Id` values to find in the first list.
The results are written back out to the second array.
This second specialization is useful for inverting index maps.

.. load-example:: DeviceAdapterAlgorithmUpperBounds
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using the ``UpperBounds`` algorithm.


------------------------------
Specifying the Device Adapter
------------------------------

.. index::
   single: algorithm; selecting device

When you call a method in :struct:`viskores::cont::Algorithm`, a device is automatically specified based on available hardware and the |Viskores| runtime state.
However, if you want to use a specific device, you can specify that device by passing either a :class:`viskores::cont::DeviceAdapterId` or a device adapter tag as the first argument to any of these methods.

.. load-example:: DeviceAdapterAlgorithmDeviceAdapter
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Using a ``DeviceAdapter`` with :struct:`viskores::cont::Algorithm`.


------------------------------
Predicates and Operators
------------------------------

.. index::
   single: predicates and operators

|Viskores| follows certain design philosophies consistent with the functional programming paradigm.
This assists in making implementations device agnostic and ensuring that various functions operate correctly and efficiently in multiple environments.
Many basic operations, such as binary and unary comparisons and predicates, are implemented as templated functors.
These are mostly re-implementations of basic C++ STL functors that can be used in the |Viskores| execution environment.

Strictly using a functor by itself adds little in the way of functionality to the code.
Their use is demonstrated more when used as parameters to one of the :struct:`viskores::cont::Algorithm` methods discussed earlier in this chapter.
Currently, |Viskores| provides 3 categories of functors: ``Unary Predicates``, ``Binary Predicates``, and ``Binary Operators``.


Unary Predicates
~~~~~~~~~~~~~~~~

.. index::
   single: predicates and operators; unary predicates

``Unary Predicates`` are functors that take a single parameter and return a boolean value.
These types of functors are useful in determining if values have been initialized or zeroed out correctly.

:struct:`viskores::IsZeroInitialized`
   Returns true if the argument is the identity of its type.

:struct:`viskores::NotZeroInitialized`
   Returns true if the argument is not the identity of its type.

:struct:`viskores::LogicalNot`
   Returns true if and only if the argument is false.
   Requires that the argument type is convertible to a boolean or implements the ``!`` operator.

.. doxygenstruct:: viskores::IsZeroInitialized
.. doxygenstruct:: viskores::NotZeroInitialized
.. doxygenstruct:: viskores::LogicalNot

.. load-example:: UnaryPredicates
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Basic unary predicates.


Binary Predicates
~~~~~~~~~~~~~~~~~

.. index::
   single: predicates and operators; binary predicates

``Binary Predicates`` take two parameters and return a single boolean value.
These types of functors are used when comparing two different parameters for some sort of condition.

:struct:`viskores::Equal`
   Returns true if and only if the first argument is equal to the second argument.
   Requires that the argument type implements the ``==`` operator.

:struct:`viskores::NotEqual`
   Returns true if and only if the first argument is not equal to the second argument.
   Requires that the argument type implements the ``!=`` operator.

:struct:`viskores::SortLess`
   Returns true if and only if the first argument is less than the second argument.
   Requires that the argument type implements the ``<`` operator.

:struct:`viskores::SortGreater`
   Returns true if and only if the first argument is greater than the second argument.
   Requires that the argument type implements the ``<`` operator; the comparison is inverted internally.

:struct:`viskores::LogicalAnd`
   Returns true if and only if the first argument and the second argument are true.
   Requires that the argument type is convertible to a boolean or implements the ``&&`` operator.

:struct:`viskores::LogicalOr`
   Returns true if and only if the first argument or the second argument is true.
   Requires that the argument type is convertible to a boolean or implements the ``||`` operator.

.. doxygenstruct:: viskores::Equal
.. doxygenstruct:: viskores::NotEqual
.. doxygenstruct:: viskores::SortLess
.. doxygenstruct:: viskores::SortGreater
.. doxygenstruct:: viskores::LogicalAnd
.. doxygenstruct:: viskores::LogicalOr

.. load-example:: BinaryPredicates
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Basic binary predicates.


Binary Operators
~~~~~~~~~~~~~~~~

.. index::
   single: predicates and operators; binary operators

``Binary Operators`` take two parameters and return a single value, usually of the same type as the input arguments.
These types of functors are useful when performing reductions or transformations of a data set.

:struct:`viskores::Sum`
   Returns the sum of two arguments.
   Requires that the argument type implements the ``+`` operator.

:struct:`viskores::Product`
   Returns the product, multiplication, of two arguments.
   Requires that the argument type implements the ``*`` operator.

:struct:`viskores::Maximum`
   Returns the larger of two arguments.
   Requires that the argument type implements the ``<`` operator.

:struct:`viskores::Minimum`
   Returns the smaller of two arguments.
   Requires that the argument type implements the ``<`` operator.

:struct:`viskores::MinAndMax`
   Returns a ``viskores::Vec<T, 2>`` that represents the minimum and maximum values.
   Requires that the argument type implements the :func:`viskores::Min` and :func:`viskores::Max` functions.

:struct:`viskores::BitwiseAnd`
   Returns the bitwise and of two arguments.
   Requires that the argument type implements the ``&`` operator.

:struct:`viskores::BitwiseOr`
   Returns the bitwise or of two arguments.
   Requires that the argument type implements the ``|`` operator.

:struct:`viskores::BitwiseXor`
   Returns the bitwise xor of two arguments.
   Requires that the argument type implements the ``^`` operator.

.. doxygenstruct:: viskores::Sum
.. doxygenstruct:: viskores::Product
.. doxygenstruct:: viskores::Maximum
.. doxygenstruct:: viskores::Minimum
.. doxygenstruct:: viskores::MinAndMax
.. doxygenstruct:: viskores::BitwiseAnd
.. doxygenstruct:: viskores::BitwiseOr
.. doxygenstruct:: viskores::BitwiseXor

.. load-example:: BinaryOperators
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Basic binary operators.


Creating Custom Comparators
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. index::
   single: predicates and operators; creating custom comparators

In addition to using the built-in operators and predicates, it is possible to create your own custom functors to be used in one of the :struct:`viskores::cont::Algorithm` methods.
Custom operator and predicate functors can be used to apply specific logic used to manipulate your data.
The following example creates a unary predicate that checks if the input is a power of 2.

.. load-example:: CustomUnaryPredicateImplementation
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Custom unary predicate implementation.

.. load-example:: CustomUnaryPredicateUsage
   :file: GuideExampleDeviceAdapterAlgorithms.cxx
   :caption: Custom unary predicate usage.
