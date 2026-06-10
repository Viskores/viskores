## Testing code consolidated

The testing helper code in Viskores has been consolidated. Originally, there was
a separation between the tests that used code in the control environment and
that which did not. This was originally from back in the day when Viskores (or
rather, its predecessor) was envisioned as a header-only library. However, this
separation is no longer meaningful. In fact, the testing code without the
control environment was missing some initialization that can cause issues with
some devices.

Now, all the testing helper code is accessible from
`viskores/testing/Testing.h`. The old `viskores/cont/testing/Testing.h` still
exists (for now), but it is an empty file pointing back to
`viskores/testing/Testing.h`.
