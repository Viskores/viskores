## Fixed bug in Kokkos Reduce function

The `Reduce()` function in the Kokkos device adapter had a potential bug where
it would attempt to access an array on the host. Normally, it is fine to request
an array handle on the host with, for example, the `ReadPortal()` method.
However, in some rare cases with custom array handles, the portal specifically
accesses functions or data on the device. This was the case for the
`CountBitSet` method in the Kokkos device adapter, and that could ead to
problems such as a segmentation fault. The `Reduce()` is now more careful to
access its input array only on the device.
