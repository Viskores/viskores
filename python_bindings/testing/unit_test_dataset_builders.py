##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import numpy as np

from viskores import CELL_SHAPE_TRIANGLE
from viskores.cont import (
    Field,
    DataSet,
    DataSetBuilderCurvilinear,
    DataSetBuilderExplicit,
    DataSetBuilderExplicitIterative,
    DataSetBuilderRectilinear,
    DataSetBuilderUniform,
)
from viskores.python_convenience import cell_set_type_name, create_uniform_dataset


def expect_raises(callable_object, *args, **kwargs):
    try:
        callable_object(*args, **kwargs)
    except Exception:
        return
    raise AssertionError("Expected an exception.")


def assert_bounds_allclose(actual, expected):
    np.testing.assert_allclose(
        (actual.X.Min, actual.X.Max, actual.Y.Min, actual.Y.Max, actual.Z.Min, actual.Z.Max),
        expected,
    )


def assert_dataset_shape(dataset, points, cells, cell_set_type, coord_name, bounds):
    assert isinstance(dataset, DataSet)
    assert dataset.GetNumberOfPoints() == points
    assert dataset.GetNumberOfCells() == cells
    assert cell_set_type_name(dataset) == cell_set_type
    assert dataset.GetCoordinateSystemName() == coord_name
    assert_bounds_allclose(dataset.GetCoordinateSystem().GetBounds(), bounds)


def test_uniform_builder_1d():
    dataset = DataSetBuilderUniform.Create(
        (4,),
        origin=(1.0, 0.0, 0.0),
        spacing=(0.5, 1.0, 1.0),
        coord_name="line_coords",
    )
    assert_dataset_shape(
        dataset,
        points=4,
        cells=3,
        cell_set_type="structured1d",
        coord_name="line_coords",
        bounds=(1.0, 2.5, 0.0, 0.0, 0.0, 0.0),
    )


def test_uniform_builder_2d():
    dataset = DataSetBuilderUniform.Create(
        (4, 3),
        origin=(1.0, 2.0, 0.0),
        spacing=(0.5, 2.0, 1.0),
        coord_name="coordinates",
    )
    assert_dataset_shape(
        dataset,
        points=12,
        cells=6,
        cell_set_type="structured2d",
        coord_name="coordinates",
        bounds=(1.0, 2.5, 2.0, 6.0, 0.0, 0.0),
    )


def test_uniform_builder_3d():
    dataset = DataSetBuilderUniform.Create(
        (3, 2, 4),
        origin=(1.0, 2.0, -1.0),
        spacing=(0.5, 2.0, 1.5),
        coord_name="volume_coords",
    )
    assert_dataset_shape(
        dataset,
        points=24,
        cells=6,
        cell_set_type="structured3d",
        coord_name="volume_coords",
        bounds=(1.0, 2.0, 2.0, 4.0, -1.0, 3.5),
    )


def test_uniform_builder_fixed_vec_arguments_require_exact_length():
    expect_raises(DataSetBuilderUniform.Create, (4, 3), origin=(1.0, 2.0))
    expect_raises(DataSetBuilderUniform.Create, (4, 3), origin=(1.0, 2.0, 3.0, 4.0))
    expect_raises(DataSetBuilderUniform.Create, (4, 3), spacing=(1.0, 2.0))
    expect_raises(DataSetBuilderUniform.Create, (4, 3), spacing=(1.0, 2.0, 3.0, 4.0))


def test_create_uniform_dataset_with_shaped_fields():
    point_scalars = np.arange(12, dtype=np.float32).reshape(4, 3)
    point_vectors = np.arange(36, dtype=np.float64).reshape(4, 3, 3)
    cell_scalars = np.arange(6, dtype=np.int32).reshape(3, 2)

    dataset = create_uniform_dataset(
        (4, 3),
        point_fields={
            "point_scalars": point_scalars,
            "point_vectors": point_vectors,
        },
        cell_fields={"cell_scalars": cell_scalars},
    )

    assert_dataset_shape(
        dataset,
        points=12,
        cells=6,
        cell_set_type="structured2d",
        coord_name="coords",
        bounds=(0.0, 3.0, 0.0, 2.0, 0.0, 0.0),
    )
    assert dataset.HasField("point_scalars", association=Field.Association.Points)
    assert dataset.HasField("point_vectors", association=Field.Association.Points)
    assert dataset.HasField("cell_scalars", association=Field.Association.Cells)
    np.testing.assert_array_equal(
        dataset.GetField("point_scalars", association=Field.Association.Points).GetData().AsNumPy(),
        point_scalars.reshape(-1),
    )
    np.testing.assert_array_equal(
        dataset.GetField("point_vectors", association=Field.Association.Points).GetData().AsNumPy(),
        point_vectors.reshape(-1, 3),
    )
    np.testing.assert_array_equal(
        dataset.GetField("cell_scalars", association=Field.Association.Cells).GetData().AsNumPy(),
        cell_scalars.reshape(-1),
    )

    point_scalars[0, 0] = 123.0
    cell_scalars[0, 0] = 99
    assert dataset.GetField("point_scalars", association=Field.Association.Points).GetData().AsNumPy()[0] == 123.0
    assert dataset.GetField("cell_scalars", association=Field.Association.Cells).GetData().AsNumPy()[0] == 99


def test_create_uniform_dataset_infers_dimensions_from_shaped_field():
    values = np.arange(6, dtype=np.float32).reshape(2, 3)
    dataset = create_uniform_dataset(point_fields={"values": values})
    assert_dataset_shape(
        dataset,
        points=6,
        cells=2,
        cell_set_type="structured2d",
        coord_name="coords",
        bounds=(0.0, 1.0, 0.0, 2.0, 0.0, 0.0),
    )
    np.testing.assert_array_equal(dataset.GetField("values").GetData().AsNumPy(), values.reshape(-1))


def test_create_uniform_dataset_rejects_bad_field_shapes():
    try:
        create_uniform_dataset((4, 3), point_fields={"bad": np.zeros((4, 4), dtype=np.float32)})
    except ValueError as error:
        assert "point field 'bad' has shape (4, 4)" in str(error)
    else:
        raise AssertionError("Expected create_uniform_dataset to reject a bad point field shape.")

    try:
        create_uniform_dataset(
            (4, 3),
            point_fields={"bad": np.zeros((4, 3), dtype=np.float32).T},
        )
    except ValueError as error:
        assert "point field 'bad' must be C-contiguous" in str(error)
    else:
        raise AssertionError("Expected create_uniform_dataset to reject a non-contiguous field.")


def test_rectilinear_builder_1d():
    dataset = DataSetBuilderRectilinear.Create([0.0, 1.0, 4.0], coord_name="line_coords")
    assert_dataset_shape(
        dataset,
        points=3,
        cells=2,
        cell_set_type="structured1d",
        coord_name="line_coords",
        bounds=(0.0, 4.0, 0.0, 0.0, 0.0, 0.0),
    )


def test_rectilinear_builder_2d():
    dataset = DataSetBuilderRectilinear.Create(
        np.array([0.0, 1.0, 3.0], dtype=np.float32),
        np.array([10.0, 12.0], dtype=np.float32),
        coord_name="plane_coords",
    )
    assert_dataset_shape(
        dataset,
        points=6,
        cells=2,
        cell_set_type="structured2d",
        coord_name="plane_coords",
        bounds=(0.0, 3.0, 10.0, 12.0, 0.0, 0.0),
    )


def test_rectilinear_builder_3d():
    dataset = DataSetBuilderRectilinear.Create(
        np.array([0.0, 1.0, 3.0], dtype=np.float64),
        np.array([10.0, 12.0], dtype=np.float64),
        np.array([-1.0, 0.0, 2.0], dtype=np.float64),
        "volume_coords",
    )
    assert_dataset_shape(
        dataset,
        points=18,
        cells=4,
        cell_set_type="structured3d",
        coord_name="volume_coords",
        bounds=(0.0, 3.0, 10.0, 12.0, -1.0, 2.0),
    )


def test_curvilinear_builder_1d():
    coords = np.asarray(
        [
            [0.0, 0.0, 0.0],
            [1.0, 0.25, 0.0],
            [2.0, 1.0, 0.0],
            [4.0, 2.0, 0.0],
        ],
        dtype=np.float32,
    )
    dataset = DataSetBuilderCurvilinear.Create(coords, (4,), coord_name="curve_coords")
    assert_dataset_shape(
        dataset,
        points=4,
        cells=3,
        cell_set_type="structured1d",
        coord_name="curve_coords",
        bounds=(0.0, 4.0, 0.0, 2.0, 0.0, 0.0),
    )
    assert dataset.GetCoordinateSystem().GetData().AsNumPy().dtype == coords.dtype


def test_curvilinear_builder_2d():
    coords = np.asarray(
        [
            [0.0, 0.0, 0.0],
            [1.0, 0.1, 0.0],
            [2.0, 0.0, 0.0],
            [0.0, 1.0, 0.0],
            [1.0, 1.2, 0.0],
            [2.0, 1.0, 0.0],
        ],
        dtype=np.float64,
    )
    dataset = DataSetBuilderCurvilinear.Create(coords, (3, 2), coord_name="surface_coords")
    assert_dataset_shape(
        dataset,
        points=6,
        cells=2,
        cell_set_type="structured2d",
        coord_name="surface_coords",
        bounds=(0.0, 2.0, 0.0, 1.2, 0.0, 0.0),
    )
    assert dataset.GetCoordinateSystem().GetData().AsNumPy().dtype == coords.dtype


def test_curvilinear_builder_3d():
    coords = np.asarray(
        [
            [x, y + 0.1 * x, z + 0.2 * y]
            for z in range(2)
            for y in range(2)
            for x in range(3)
        ],
        dtype=np.float32,
    )
    dataset = DataSetBuilderCurvilinear.Create(coords, (3, 2, 2), coord_name="volume_coords")
    assert_dataset_shape(
        dataset,
        points=12,
        cells=2,
        cell_set_type="structured3d",
        coord_name="volume_coords",
        bounds=(0.0, 2.0, 0.0, 1.2, 0.0, 1.2),
    )
    assert dataset.GetCoordinateSystem().GetData().AsNumPy().dtype == coords.dtype


def test_explicit_iterative_builder():
    builder = DataSetBuilderExplicitIterative()
    builder.Begin("iterative_coords")
    builder.AddPoint((0.0, 0.0, 0.0))
    builder.AddPoint((1.0, 0.0, 0.0))
    builder.AddPoint((0.0, 1.0, 0.0))
    builder.AddCell(CELL_SHAPE_TRIANGLE, (0, 1, 2))
    dataset = builder.Create()
    assert_dataset_shape(
        dataset,
        points=3,
        cells=1,
        cell_set_type="explicit",
        coord_name="iterative_coords",
        bounds=(0.0, 1.0, 0.0, 1.0, 0.0, 0.0),
    )
    assert dataset.GetCellSet().GetCellPointIds(0) == [0, 1, 2]


def test_explicit_iterative_builder_point_requires_exact_vec3():
    builder = DataSetBuilderExplicitIterative()
    builder.Begin("iterative_coords")
    expect_raises(builder.AddPoint, (0.0, 0.0))
    expect_raises(builder.AddPoint, (0.0, 0.0, 0.0, 0.0))


def test_explicit_builder_numpy_inputs():
    coords = np.asarray(
        [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
        dtype=np.float64,
    )
    dataset = DataSetBuilderExplicit.Create(
        coords,
        np.asarray([CELL_SHAPE_TRIANGLE], dtype=np.uint8),
        np.asarray([3], dtype=np.int32),
        np.asarray([0, 1, 2], dtype=np.int64),
        coord_name="explicit_coords",
    )
    assert_dataset_shape(
        dataset,
        points=3,
        cells=1,
        cell_set_type="explicit",
        coord_name="explicit_coords",
        bounds=(0.0, 1.0, 0.0, 1.0, 0.0, 0.0),
    )
    assert dataset.GetCoordinateSystem().GetData().AsNumPy().dtype == coords.dtype


def main():
    test_uniform_builder_1d()
    test_uniform_builder_2d()
    test_uniform_builder_3d()
    test_uniform_builder_fixed_vec_arguments_require_exact_length()
    test_create_uniform_dataset_with_shaped_fields()
    test_create_uniform_dataset_infers_dimensions_from_shaped_field()
    test_create_uniform_dataset_rejects_bad_field_shapes()
    test_rectilinear_builder_1d()
    test_rectilinear_builder_2d()
    test_rectilinear_builder_3d()
    test_curvilinear_builder_1d()
    test_curvilinear_builder_2d()
    test_curvilinear_builder_3d()
    test_explicit_iterative_builder()
    test_explicit_iterative_builder_point_requires_exact_vec3()
    test_explicit_builder_numpy_inputs()


if __name__ == "__main__":
    main()
