# Add support for BitFields.

BitFields are:
- Stored in memory using a contiguous buffer of bits.
- Accessible via portals, a la ArrayHandle.
- Portals operate on individual bits or words.
- Operations may be atomic for safe use from concurrent kernels.

The new BitFieldToUnorderedSet device algorithm produces an
ArrayHandle containing the indices of all set bits, in no particular
order.

The new AtomicInterface classes provide an abstraction into bitwise
atomic operations across control and execution environments and are
used to implement the BitPortals.
