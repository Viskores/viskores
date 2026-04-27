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

import viskores.cont
from viskores.cont import Association, CoordinateSystem, DataSet, Field


def expect_raises(callable_object, *args):
    try:
        callable_object(*args)
    except Exception:
        return
    raise AssertionError("Expected an exception.")


def test_coordinate_systems():
    dataset = DataSet()
    points = np.asarray(
        [[0.0, 0.0, 0.0], [1.0, 2.0, 3.0], [4.0, 5.0, 6.0]],
        dtype=np.float32,
    )

    coords = CoordinateSystem("coords", points)
    dataset.AddCoordinateSystem(coords)
    assert dataset.GetNumberOfCoordinateSystems() == 1
    assert dataset.HasCoordinateSystem("coords")
    assert dataset.GetCoordinateSystemName() == "coords"
    np.testing.assert_allclose(dataset.GetCoordinateSystem().GetData().AsNumPy(), points)
    np.testing.assert_allclose(dataset.GetCoordinateSystem("coords").GetData().AsNumPy(), points)
    np.testing.assert_allclose(dataset.GetCoordinateSystem(0).GetData().AsNumPy(), points)

    alternate = np.asarray([[1.0, 0.0, 0.0], [2.0, 0.0, 0.0], [3.0, 0.0, 0.0]], dtype=np.float64)
    dataset.AddCoordinateSystem("alternate", alternate)
    assert dataset.GetNumberOfCoordinateSystems() == 2
    assert dataset.GetCoordinateSystemName(1) == "alternate"
    assert dataset.GetCoordinateSystem("alternate").GetData().AsNumPy().dtype == alternate.dtype
    np.testing.assert_allclose(dataset.GetCoordinateSystem("alternate").GetData().AsNumPy(), alternate)


def test_fields_and_associations():
    dataset = DataSet()
    dataset.AddCoordinateSystem(
        "coords",
        np.asarray([[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [2.0, 0.0, 0.0]], dtype=np.float32),
    )

    point_values = np.asarray([10.0, 20.0, 30.0], dtype=np.float32)
    cell_values = np.asarray([4, 5], dtype=np.int32)
    whole_values = np.asarray([42.0], dtype=np.float64)

    dataset.AddPointField("point_values", point_values)
    dataset.AddCellField("cell_values", cell_values)
    dataset.AddField(Field("whole_values", Association.WHOLE_DATASET, whole_values))

    assert dataset.GetNumberOfFields() == 4
    assert dataset.HasField("coords", Association.POINTS)
    assert dataset.HasField("point_values")
    assert dataset.HasField("point_values", Association.POINTS)
    assert not dataset.HasField("point_values", Association.CELLS)
    assert dataset.HasField("cell_values", "cells")
    assert dataset.HasField("whole_values", "whole_dataset")
    assert set(dataset.FieldNames()) == {"coords", "point_values", "cell_values", "whole_values"}

    np.testing.assert_array_equal(dataset.GetField("point_values"), point_values)
    np.testing.assert_array_equal(dataset.GetField("cell_values", "cells"), cell_values)
    np.testing.assert_array_equal(dataset.GetField("whole_values"), whole_values)

    field_object = dataset.GetFieldObject("point_values", Association.POINTS)
    assert isinstance(field_object, Field)
    assert field_object.GetName() == "point_values"
    assert field_object.IsPointField()
    np.testing.assert_array_equal(field_object.GetData(), point_values)

    expect_raises(dataset.GetField, "point_values", Association.CELLS)
    expect_raises(dataset.GetFieldObject, "missing")


def test_ghost_cell_field():
    dataset = DataSet()
    dataset.AddCoordinateSystem(
        "coords",
        np.asarray([[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [2.0, 0.0, 0.0]], dtype=np.float32),
    )
    ghost_values = np.asarray([0, 1], dtype=np.uint8)

    assert not dataset.HasGhostCellField()
    dataset.SetGhostCellField("ghosts", ghost_values)
    assert dataset.HasGhostCellField()
    assert dataset.GetGhostCellFieldName() == "ghosts"
    np.testing.assert_array_equal(dataset.GetGhostCellField(), ghost_values)

    default_named_ghost_values = np.asarray([1, 0], dtype=np.uint8)
    dataset.SetGhostCellField(default_named_ghost_values)
    assert dataset.HasGhostCellField()
    np.testing.assert_array_equal(dataset.GetGhostCellField(), default_named_ghost_values)


def main():
    viskores.cont.Initialize(["unit_test_dataset_class.py"])
    test_coordinate_systems()
    test_fields_and_associations()
    test_ghost_cell_field()


if __name__ == "__main__":
    main()
