## Added check for Kokkos finalize

Previously, if Viskores initialized Kokkos, it would always fall the Kokkos
finalize routine. This assumed that all other Kokkos users would follow the same
pattern of checking before initializing Kokkos and finalizing if and only if
Viskores initialized.

However, a more aggressive library may always finalize on shutdown if it detects
Kokkos was ever initialized. To play well with such libraries, Viskores now also
checks to make sure that Kokkos has not yet been finalized.

This cannot guarantee that another library may finalize Kokkos after Viskores,
but it will work with any libraries that either only finalize if they do the
initialization or check before initializing or finalizing.
