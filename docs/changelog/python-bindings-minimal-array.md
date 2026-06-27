## Add minimal Python bindings for UnknownArrayHandle

Viskores now provides Python bindings for `viskores::cont::UnknownArrayHandle`
and zero-copy conversion to and from NumPy:

* `viskores.cont.array_from_numpy(array, allow_copy=False)` wraps a NumPy
  array as a `UnknownArrayHandle`, sharing storage when the input is
  C-contiguous, aligned, and writable. Pass `allow_copy=True` to let the
  binding make a contiguous, writable copy when the input layout cannot be
  shared directly. Supported on 1D scalar and 2D vector NumPy arrays for
  every dtype that has a Viskores scalar equivalent.

* `viskores.cont.asnumpy(array)` (and the `asnumpy` method on
  `UnknownArrayHandle`) returns a read-only NumPy view of any basic or
  runtime-vec `ArrayHandle`. Pointer stability is provided by
  `Buffer::PinHost`, so the view is zero-copy even when the source buffer
  was allocated by Viskores rather than by NumPy. Other array layouts are
  resolved through `CastAndCallForTypes` and copied into a fresh NumPy
  array.

The build is opt-in via `Viskores_ENABLE_PYTHON_BINDINGS=ON` and
optionally targets the CPython stable ABI with
`VISKORES_PYTHON_STABLE_ABI=ON` (requires Python 3.12 and CMake 3.26).
