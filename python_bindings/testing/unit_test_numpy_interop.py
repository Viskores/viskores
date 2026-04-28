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


def group_vec_class_name(dtype):
    dtype = np.dtype(dtype)
    if dtype == np.float32:
        suffix = "Float32"
    elif dtype == np.float64:
        suffix = "Float64"
    elif dtype.kind == "i":
        suffix = f"Int{dtype.itemsize * 8}"
    elif dtype.kind == "u":
        suffix = f"UInt{dtype.itemsize * 8}"
    else:
        raise AssertionError(f"unexpected dtype {dtype}")
    return f"ArrayHandleGroupVecVariable{suffix}"


def assert_group_values(groups, expected):
    assert isinstance(groups, list)
    assert len(groups) == len(expected)
    for actual, expected_group in zip(groups, expected):
        np.testing.assert_array_equal(actual, expected_group)


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


def check_group_vec_dtype(dtype):
    components = np.arange(9, dtype=dtype)
    offsets = np.array([0, 4, 6, 9], dtype=np.int64)
    expected = [components[0:4], components[4:6], components[6:9]]

    group_vec_class = getattr(viskores_cont, group_vec_class_name(dtype))
    group_vec = group_vec_class(components, offsets)
    assert "ArrayHandleGroupVecVariable" in repr(group_vec)
    assert len(group_vec) == 3
    assert group_vec.GetNumberOfValues() == 3
    assert group_vec.GetNumberOfComponentsFlat() == 0
    np.testing.assert_array_equal(group_vec.GetComponentsArray().AsNumPy(), components)
    np.testing.assert_array_equal(group_vec.GetOffsetsArray().AsNumPy(), offsets)
    np.testing.assert_array_equal(group_vec[0], expected[0])
    np.testing.assert_array_equal(group_vec[-1], expected[-1])
    assert_group_values(group_vec.AsList(), expected)
    assert_group_values(group_vec.AsNumPy(), expected)
    assert_group_values(asnumpy(group_vec), expected)

    field = Field(f"group_vec_{np.dtype(dtype).name}", Association.WHOLE_DATASET, group_vec)
    assert field.GetNumberOfValues() == 3
    assert_group_values(field.GetData(), expected)


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
        check_group_vec_dtype(dtype)
        for components in (2, 3, 4):
            check_vector_dtype(dtype, components)
            check_soa_dtype(dtype, components)

    id_group = viskores_cont.ArrayHandleGroupVecVariableId(
        np.arange(3, dtype=np.int64), np.array([0, 1, 3], dtype=np.int64)
    )
    assert_group_values(id_group.AsList(), [np.array([0], dtype=np.int64), np.array([1, 2])])

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
