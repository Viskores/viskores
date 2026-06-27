##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

import gc

import numpy as np

import viskores
import viskores.cont


# Keep this list aligned with the C++ dtype table in cont_bindings.cxx.
SCALAR_DTYPES = [
    np.int8,
    np.int16,
    np.int32,
    np.int64,
    np.uint8,
    np.uint16,
    np.uint32,
    np.uint64,
    np.float32,
    np.float64,
]


def scalar_values(dtype):
    if np.issubdtype(dtype, np.floating):
        return np.linspace(-2.5, 3.5, 7, dtype=dtype)
    if np.issubdtype(dtype, np.unsignedinteger):
        return np.arange(7, dtype=dtype)
    return np.arange(-3, 4, dtype=dtype)


def vector_values(dtype, components):
    return np.arange(4 * components, dtype=dtype).reshape(4, components)


def check_roundtrip(values):
    # Exercise both the bound method and module-level helper for dense scalar and
    # runtime-component vector arrays.
    unknown = viskores.cont.array_from_numpy(values)
    assert isinstance(unknown, viskores.cont.UnknownArrayHandle)
    assert unknown.GetNumberOfValues() == values.shape[0]
    assert unknown.GetNumberOfComponentsFlat() == (1 if values.ndim == 1 else values.shape[1])

    method_values = unknown.asnumpy()
    function_values = viskores.cont.asnumpy(unknown)
    assert method_values.dtype == values.dtype
    assert function_values.dtype == values.dtype
    np.testing.assert_array_equal(method_values, values)
    np.testing.assert_array_equal(function_values, values)


def mutation_value(dtype, value):
    return np.asarray(value, dtype=dtype).item()


def check_dense_shared_storage(values):
    # Dense NumPy input and output should share storage with Viskores. The
    # returned view is read-only so multiple Python views can coexist while
    # Viskores holds read access to the array.
    unknown = viskores.cont.array_from_numpy(values)
    result = unknown.asnumpy()
    assert np.shares_memory(result, values)
    assert not result.flags.writeable

    values.reshape(-1)[0] = mutation_value(values.dtype, 7)
    assert result.reshape(-1)[0] == values.reshape(-1)[0]

    try:
        result.reshape(-1)[-1] = mutation_value(values.dtype, 3)
    except ValueError:
        pass
    else:
        raise AssertionError("Expected asnumpy() view to be read-only.")


def check_dense_lifetime(values):
    # The UnknownArrayHandle should keep NumPy-owned input storage alive, and a
    # NumPy view returned from asnumpy should keep the Viskores array alive.
    expected = values.copy()
    unknown = viskores.cont.array_from_numpy(values)
    del values
    gc.collect()

    result = unknown.asnumpy()
    np.testing.assert_array_equal(result, expected)

    del unknown
    gc.collect()
    np.testing.assert_array_equal(result, expected)


def check_double_wrap_keeps_view_path():
    # Wrapping the same NumPy array twice must keep both wrappers on the
    # zero-copy view path: the second wrapper must continue to produce a
    # zero-copy view of the same NumPy buffer after the first wrapper is
    # destroyed.
    # Use a sufficiently large array so any chance reuse of the same allocation
    # by numpy.empty cannot fool np.shares_memory.
    base = np.arange(1024 * 1024, dtype=np.float32)
    u1 = viskores.cont.array_from_numpy(base)
    u2 = viskores.cont.array_from_numpy(base)
    del u1
    gc.collect()
    view = u2.asnumpy()
    assert np.shares_memory(view, base), \
        "Second wrapper must still produce a zero-copy view after the first is gone."
    # Mutating the source must propagate through the view.
    base[0] = -1.0
    assert view[0] == -1.0


def check_view_outlives_both_source_and_unknown(values):
    # Strongest lifetime check: drop both the Python NumPy source and the
    # UnknownArrayHandle in one step. The view returned by asnumpy must still
    # be readable because its capsule holds the Viskores ArrayHandle (which
    # holds the PyBufferOwner that owns the buffer).
    expected = values.copy()
    unknown = viskores.cont.array_from_numpy(values)
    view = unknown.asnumpy()
    del values, unknown
    gc.collect()
    np.testing.assert_array_equal(view, expected)


def check_rejects_nonshareable_arrays(dtype):
    # Both 1D scalar and 2D vector inputs share the same shareability checks,
    # so exercise both shapes for the non-contiguous and read-only cases.
    for values in (scalar_values(dtype), vector_values(dtype, 3)):
        # A non-contiguous slice of either shape must be rejected without allow_copy=True.
        try:
            viskores.cont.array_from_numpy(values[::2])
        except RuntimeError:
            pass
        else:
            raise AssertionError("Expected non-contiguous input to be rejected.")

        readonly = values.copy()
        readonly.flags.writeable = False
        try:
            viskores.cont.array_from_numpy(readonly)
        except RuntimeError:
            pass
        else:
            raise AssertionError("Expected read-only input to be rejected.")


def check_rejects_non_dlpack_kinds():
    # All dtype kinds that lack a DLPack representation must be rejected with
    # a clean message before reaching the nb::ndarray cast (which would surface
    # them as an opaque std::bad_cast). Object dtype is one example; structured
    # records, byte/unicode strings, and datetime/timedelta are others.
    values = np.empty(2, dtype=object)
    values[0] = np.asarray([1, 2], dtype=np.int32)
    values[1] = np.asarray([3], dtype=np.int32)
    cases = [
        values,
        np.array([(1, 2.0)], dtype=[("a", "i4"), ("b", "f4")]),  # structured
        np.array(["hello", "world"]),                              # unicode str
        np.array([b"hi", b"bye"]),                                  # byte string
        np.array(["2024-01-01"], dtype="datetime64[D]"),
        np.array([1], dtype="timedelta64[D]"),
    ]
    for arr in cases:
        try:
            viskores.cont.array_from_numpy(arr)
        except RuntimeError as error:
            assert "are not supported" in str(error), (
                f"Expected dtype-rejection message for {arr.dtype}, got: {error}")
        else:
            raise AssertionError(
                f"Expected dtype {arr.dtype} (kind={arr.dtype.kind}) to be rejected.")


def check_rejects_unsupported_shapes():
    # The binding accepts only 1D scalar and 2D vector inputs. Other ranks
    # (0-dim scalars, 3D and higher) must be rejected with the shape-error
    # message so users understand the constraint.
    for unsupported in (np.asarray(5), np.zeros((2, 3, 4)), np.zeros((2, 3, 4, 5))):
        try:
            viskores.cont.array_from_numpy(unsupported)
        except RuntimeError:
            pass
        else:
            raise AssertionError(
                f"Expected ndim={unsupported.ndim} input to be rejected.")


def check_rejects_default_constructed():
    # A default-constructed UnknownArrayHandle holds no array. Calling asnumpy
    # on it must raise a Python error (not segfault inside Viskores).
    empty = viskores.cont.UnknownArrayHandle()
    try:
        empty.asnumpy()
    except RuntimeError:
        pass
    else:
        raise AssertionError("Expected asnumpy on empty UnknownArrayHandle to be rejected.")


def check_rejects_non_native_byte_order():
    # Non-native byte-order dtypes (e.g., '>f4' on a little-endian host) cannot
    # be represented in DLPack and must be rejected with a clean message rather
    # than the std::bad_cast that nanobind would otherwise raise.
    # Construct an array with a swapped byte-order dtype. dtype.newbyteorder("S")
    # returns the dtype with the opposite byte order; astype reinterprets the
    # values into that dtype.
    swapped_dtype = np.dtype(np.float32).newbyteorder("S")
    swapped = np.arange(8, dtype=np.float32).astype(swapped_dtype)
    if swapped.dtype.isnative:
        # Skip on platforms where '<f4' and '>f4' are both native (none today,
        # but be defensive).
        return
    try:
        viskores.cont.array_from_numpy(swapped)
    except RuntimeError as error:
        assert "byte-order" in str(error), \
            f"Expected byte-order error message, got: {error}"
    else:
        raise AssertionError("Expected non-native byte-order input to be rejected.")


def check_rejects_unsupported_dtypes():
    # Boolean, half-precision float, and complex arrays have no Viskores scalar
    # equivalent in the dispatch table. The error message should name the dtype
    # in its human-readable form (bool/float16/complex128, not |b1/<f2/<c16).
    cases = [
        (np.array([True, False]), "bool"),
        (np.array([1, 2], dtype=np.float16), "float16"),
        (np.array([1 + 2j, 3 + 4j], dtype=np.complex128), "complex128"),
    ]
    for arr, expected_name in cases:
        try:
            viskores.cont.array_from_numpy(arr)
        except RuntimeError as error:
            assert expected_name in str(error), (
                f"Expected dtype name {expected_name!r} in error message, got: {error}")
        else:
            raise AssertionError(f"Expected dtype {expected_name} to be rejected.")


def check_empty_arrays():
    # Zero-length arrays must round-trip without crashing. Both 1D and 2D
    # forms are exercised because they take different code paths in the
    # binding (ArrayHandleBasic vs. ArrayHandleRuntimeVec).
    for dtype in (np.float32, np.int64, np.uint8):
        empty_1d = np.empty(0, dtype=dtype)
        unknown = viskores.cont.array_from_numpy(empty_1d)
        assert unknown.GetNumberOfValues() == 0
        result = unknown.asnumpy()
        assert result.shape == (0,)
        assert result.dtype == dtype

        empty_2d = np.empty((0, 3), dtype=dtype)
        unknown = viskores.cont.array_from_numpy(empty_2d)
        assert unknown.GetNumberOfValues() == 0
        assert unknown.GetNumberOfComponentsFlat() == 3
        result = unknown.asnumpy()
        assert result.shape == (0, 3)
        assert result.dtype == dtype


def check_copy_kwarg_accepts_unshareable(dtype):
    # allow_copy=True must accept non-contiguous and read-only inputs that the default
    # allow_copy=False rejects, and the resulting ArrayHandle must round-trip.
    for values in (scalar_values(dtype), vector_values(dtype, 3)):
        sliced = values[::2]
        unknown = viskores.cont.array_from_numpy(sliced, allow_copy=True)
        np.testing.assert_array_equal(unknown.asnumpy(), sliced)
        # allow_copy=True breaks the storage link: mutating the source must not affect
        # the resulting ArrayHandle's view.
        expected = unknown.asnumpy().copy()
        values.reshape(-1)[0] = mutation_value(dtype, 99)
        np.testing.assert_array_equal(unknown.asnumpy(), expected)

        readonly = values.copy()
        readonly.flags.writeable = False
        unknown = viskores.cont.array_from_numpy(readonly, allow_copy=True)
        np.testing.assert_array_equal(unknown.asnumpy(), readonly)


def check_copy_kwarg_does_not_force_copy(dtype):
    # allow_copy=True is permissive, not mandatory: a directly-shareable input still
    # gets the zero-copy path and continues to share storage with the result.
    for values in (scalar_values(dtype), vector_values(dtype, 3)):
        unknown = viskores.cont.array_from_numpy(values, allow_copy=True)
        assert np.shares_memory(unknown.asnumpy(), values)


def check_source_mutation_propagates(values):
    # Writes through the source NumPy array must be visible through both the
    # ArrayHandle and any view returned by asnumpy. This is the documented
    # shared-storage behavior for arrays created via array_from_numpy.
    unknown = viskores.cont.array_from_numpy(values)
    view = unknown.asnumpy()
    flat_source = values.reshape(-1)
    flat_view = view.reshape(-1)
    flat_source[0] = mutation_value(values.dtype, 42)
    assert flat_view[0] == flat_source[0]


def check_viskores_allocated_zero_copy(values):
    # Produces an UnknownArrayHandle whose buffer was allocated by Viskores
    # (no PyBufferOwner). The asnumpy zero-copy path for this case relies on
    # Buffer::PinHost; verify that the returned NumPy view really does share
    # storage with the source ArrayHandle, that two views over the same array
    # see the same memory, and that a view outlives the originating
    # UnknownArrayHandle.
    expected = values.copy()
    source = viskores.cont.array_from_numpy(values)
    allocated = source.NewInstanceBasic()
    allocated.DeepCopyFrom(source)
    del source
    gc.collect()

    view_a = allocated.asnumpy()
    view_b = allocated.asnumpy()
    np.testing.assert_array_equal(view_a, expected)
    np.testing.assert_array_equal(view_b, expected)
    assert np.shares_memory(view_a, view_b), \
        "Both asnumpy views of a Viskores-allocated array must share storage."
    assert not view_a.flags.writeable

    # The view's capsule holds the ArrayHandle, so the view must outlive the
    # UnknownArrayHandle that produced it.
    del allocated, view_b
    gc.collect()
    np.testing.assert_array_equal(view_a, expected)


def main():
    for dtype in SCALAR_DTYPES:
        check_roundtrip(scalar_values(dtype))
        check_dense_shared_storage(scalar_values(dtype))
        check_dense_lifetime(scalar_values(dtype))
        check_view_outlives_both_source_and_unknown(scalar_values(dtype))
        check_rejects_nonshareable_arrays(dtype)

    for dtype in SCALAR_DTYPES:
        for components in (1, 2, 3, 4, 5):
            check_roundtrip(vector_values(dtype, components))
            check_dense_shared_storage(vector_values(dtype, components))
            check_dense_lifetime(vector_values(dtype, components))
            check_view_outlives_both_source_and_unknown(vector_values(dtype, components))

    check_rejects_non_dlpack_kinds()
    check_rejects_unsupported_shapes()
    check_rejects_unsupported_dtypes()
    check_rejects_default_constructed()
    check_rejects_non_native_byte_order()
    check_double_wrap_keeps_view_path()
    check_empty_arrays()
    for dtype in (np.float32, np.int64):
        check_source_mutation_propagates(scalar_values(dtype))
        check_source_mutation_propagates(vector_values(dtype, 3))
        check_copy_kwarg_accepts_unshareable(dtype)
        check_copy_kwarg_does_not_force_copy(dtype)
        check_viskores_allocated_zero_copy(scalar_values(dtype))
        check_viskores_allocated_zero_copy(vector_values(dtype, 3))


if __name__ == "__main__":
    main()
