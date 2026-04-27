##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

import numpy as np

import viskores.cont as viskores_cont
from viskores.cont import (
    ArrayCopy,
    Association,
    CoordinateSystem,
    DataSetBuilderExplicit,
    Field,
    array_from_numpy,
    asnumpy,
)


def check_scalar_dtype(dtype):
    values = np.arange(6, dtype=dtype)
    field = Field(f"values_{np.dtype(dtype).name}", Association.POINTS, values)
    returned = field.GetData()
    assert returned.dtype == values.dtype
    np.testing.assert_array_equal(returned, values)


def check_vector_dtype(dtype, components):
    values = np.arange(12, dtype=dtype).reshape(12 // components, components)
    field = Field(f"vectors_{np.dtype(dtype).name}_{components}", Association.POINTS, values)
    returned = field.GetData()
    assert returned.dtype == values.dtype
    assert returned.shape == values.shape
    np.testing.assert_array_equal(returned, values)

    recombined = array_from_numpy(values).ExtractArrayFromComponents()
    recombined_array = recombined.AsNumPy()
    assert recombined_array.dtype == values.dtype
    np.testing.assert_array_equal(recombined_array, values)
    np.testing.assert_array_equal(asnumpy(recombined), values)

    destination_class = getattr(viskores_cont, soa_class_name(dtype, components))
    destination = destination_class()
    ArrayCopy(recombined, destination)
    np.testing.assert_array_equal(destination.AsNumPy(), values)

    field = Field(
        f"vectors_from_recombined_{np.dtype(dtype).name}_{components}",
        Association.POINTS,
        recombined,
    )
    np.testing.assert_array_equal(field.GetData(), values)


def soa_class_name(dtype, components):
    dtype = np.dtype(dtype)
    if dtype.kind == "f":
        suffix = "f_32" if dtype.itemsize == 4 else "f_64"
    elif dtype.kind == "i":
        suffix = f"i_{dtype.itemsize * 8}"
    elif dtype.kind == "u":
        suffix = f"ui_{dtype.itemsize * 8}"
    else:
        raise AssertionError(f"unexpected dtype {dtype}")
    return f"ArrayHandleSOAVec{components}{suffix}"


def check_soa_dtype(dtype, components):
    values = np.arange(12, dtype=dtype).reshape(12 // components, components)
    destination_class = getattr(viskores_cont, soa_class_name(dtype, components))
    destination = destination_class()
    ArrayCopy(array_from_numpy(values), destination)
    returned = destination.AsNumPy()
    assert returned.dtype == values.dtype
    np.testing.assert_array_equal(returned, values)

    field = Field(
        f"vectors_from_soa_{np.dtype(dtype).name}_{components}",
        Association.POINTS,
        destination,
    )
    np.testing.assert_array_equal(field.GetData(), values)


def main():
    dtypes = (
        np.int8,
        np.uint8,
        np.int16,
        np.uint16,
        np.int32,
        np.uint32,
        np.int64,
        np.uint64,
        np.float32,
        np.float64,
    )

    for dtype in dtypes:
        check_scalar_dtype(dtype)
        for components in (2, 3, 4):
            check_vector_dtype(dtype, components)
            check_soa_dtype(dtype, components)

    source = np.arange(6, dtype=np.float32).reshape(3, 2)
    copied = array_from_numpy(source)
    source[0, 0] = 99.0
    assert asnumpy(copied)[0, 0] == 0.0

    aliased = array_from_numpy(source, copy=False)
    view = aliased.AsNumPy(copy=False)
    assert view.dtype == source.dtype
    assert view.shape == source.shape
    assert not view.flags.writeable
    assert np.shares_memory(view, source)
    source[0, 1] = 42.0
    assert view[0, 1] == 42.0

    shapes = np.array([5], dtype=np.uint8)
    num_indices = np.array([3], dtype=np.int32)
    connectivity = np.array([0, 1, 2], dtype=np.int64)
    for dtype in (np.float32, np.float64):
        coords = np.asarray([[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=dtype)
        dataset = DataSetBuilderExplicit.Create(coords, shapes, num_indices, connectivity)
        returned = dataset.GetCoordinateSystem().GetData().AsNumPy()
        assert returned.dtype == coords.dtype
        np.testing.assert_array_equal(returned, coords)

        recombined = array_from_numpy(coords).ExtractArrayFromComponents()
        coordinate_system = CoordinateSystem("coords", coords)
        coordinate_system.SetData(recombined)
        np.testing.assert_array_equal(asnumpy(coordinate_system.GetData()), coords)


if __name__ == "__main__":
    main()
