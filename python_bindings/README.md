# Minimal Python Bindings

This directory contains an initial manual Python binding slice for
`viskores::cont::UnknownArrayHandle` NumPy input/output.

The current binding surface includes:

- `viskores.cont.UnknownArrayHandle`
- `viskores.cont.array_from_numpy`
- `viskores.cont.asnumpy`

Dense scalar and runtime-vector arrays can be viewed as NumPy arrays.

## NumPy Ownership

`viskores.cont.array_from_numpy` shares storage with dense NumPy arrays rather
than copying them. Viskores does not delete the NumPy array memory directly.
Instead, the binding gives the Viskores `ArrayHandleBasic` a small owner object
that keeps the Python array alive for as long as Viskores can access its data.

The lifetime chain is:

1. The binding creates a `PyBufferOwner` for the NumPy array.
2. `PyBufferOwner` stores a Python reference to the NumPy array, increasing the
   array's reference count.
3. `PyBufferOwner` also acquires a writable Python buffer export for the array
   data.
4. `ArrayHandleBasic` receives the raw buffer pointer, the `PyBufferOwner`
   pointer as its container, and a deleter callback.
5. When the last Viskores buffer reference is destroyed, Viskores calls the
   deleter callback with the `PyBufferOwner` pointer.
6. The deleter acquires Python's Global Interpreter Lock and deletes
   `PyBufferOwner`.
7. `PyBufferOwner` releases the Python buffer export and drops its Python
   reference to the NumPy array.
8. If no other Python references remain, NumPy can then destroy the array and
   release its memory.

Destroying one Python `UnknownArrayHandle` object does not necessarily release
the NumPy array immediately. Other Viskores handles that share the same buffer,
such as copied `UnknownArrayHandle` objects or derived `ArrayHandle` objects,
keep the owner alive until the last shared Viskores buffer reference is gone.

## Non-Shareable Inputs

By default `array_from_numpy(arr)` requires the NumPy array to be C-contiguous,
aligned, and writable so its storage can be shared with Viskores. Inputs that
do not meet those requirements (e.g., slices like `arr[::2]`, or read-only
arrays) are rejected with a descriptive error.

Pass `allow_copy=True` to allow the binding to make a contiguous, writable copy
automatically:

```python
unknown = viskores.cont.array_from_numpy(arr[::2], allow_copy=True)
```

`allow_copy=True` is permissive, not mandatory: when the input is already shareable,
the binding still uses the zero-copy path. When a copy is made, the original
NumPy array is no longer connected to the resulting `UnknownArrayHandle`.

## Mutability and Resizing

Sharing storage means writes are visible across the boundary in both directions:

- The source NumPy array remains writable. Mutations through it are immediately
  visible to Viskores. NumPy views returned by `asnumpy` are read-only so
  Python code cannot accidentally mutate Viskores-owned data through a view; to
  modify the data, mutate the original NumPy array.
- A Viskores filter or worklet that writes through the shared `ArrayHandle`
  mutates the original NumPy array in place. If you do not want filter writes
  to land in your input array, copy the array before passing it through
  `array_from_numpy`.

Buffers created from NumPy cannot be resized through the Viskores
`ArrayHandle`. Calls that would reallocate the buffer (for example
`ArrayHandle::Allocate` with a different size) raise
`viskores::cont::ErrorBadAllocation`. The Python NumPy array owns the
allocation, so Viskores is not allowed to free or replace it.

## NumPy Output: Zero-Copy View

`viskores.cont.UnknownArrayHandle.asnumpy` and `viskores.cont.asnumpy` return a
zero-copy NumPy view of any basic or runtime-vec `ArrayHandle`, regardless of
whether the buffer was originally created from NumPy or allocated by Viskores
itself (for example, the output of a filter). Other array layouts are resolved
through `CastAndCallForTypes` and always copied.

Pointer stability for the view is provided by `Buffer::PinHost`, which the
binding calls on the source buffer before constructing the view. PinHost is
irreversible: once an `ArrayHandle` has been exposed to NumPy through
`asnumpy`, its host buffer is locked in place for the remainder of the
program. Any subsequent attempt to grow that buffer from the Viskores side
raises `ErrorBadAllocation` rather than silently relocating the pointer and
invalidating the NumPy view.

## Stable ABI (optional)

Pass `-DVISKORES_PYTHON_STABLE_ABI=ON` at configure time to build the
extension against CPython's stable ABI (`abi3`). The resulting `.so` then
loads on any Python interpreter at or above the version it was built
against, without rebuilding.

This option requires Python 3.12 or newer and CMake 3.26 or newer; both
are prerequisites of CMake's `Python::SABIModule` target. The binding's
`PyObject_GetBuffer` / `PyBuffer_Release` calls have been part of the
Python Limited API since 3.11, so they remain valid on every supported
Python version.

## Building and Running

Enable the bindings with:

```sh
cmake -S . -B <build-dir> -DViskores_ENABLE_PYTHON_BINDINGS=ON
cmake --build <build-dir> --target viskores_python_bindings
```

Run the examples with:

```sh
PYTHONPATH=<build-dir>/python_bindings python python_bindings/examples/roundtrip_scalar_dtypes.py
PYTHONPATH=<build-dir>/python_bindings python python_bindings/examples/roundtrip_vectors.py
PYTHONPATH=<build-dir>/python_bindings python python_bindings/examples/roundtrip_allow_copy.py
```
