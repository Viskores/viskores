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

from viskores.cont import Association, DataSetBuilderExplicit, PartitionedDataSet, create_uniform_dataset
from viskores.filter.contour import Contour
from viskores.filter.geometry_refinement import Triangulate
from viskores.filter.multi_block import MergeDataSets


def create_single_cell_set_data(coords):
    dataset = DataSetBuilderExplicit.Create(
        coords,
        [5, 5],
        [3, 3],
        [0, 1, 2, 1, 2, 3],
    )
    dataset.AddPointField("pointVar", np.array([15.0, 16.0, 17.0, 18.0], dtype=np.float32))
    dataset.AddCellField("cellVar", np.array([132.0, 133.0], dtype=np.float32))
    return dataset


def create_uniform_data(origin):
    dataset = create_uniform_dataset((3, 2), origin=origin, spacing=(1, 1))
    dataset.AddPointField("pointVar", np.array([10.1, 20.1, 30.1, 40.1, 50.1, 60.1], dtype=np.float32))
    dataset.AddCellField("cellVar", np.array([100.1, 200.1], dtype=np.float32))
    return dataset


def test_uniform_same_fields_same_data_type():
    input_data_sets = PartitionedDataSet()
    input_data_sets.AppendPartition(create_uniform_data((0.0, 0.0)))
    input_data_sets.AppendPartition(create_uniform_data((3.0, 0.0)))

    result = MergeDataSets().Execute(input_data_sets).GetPartition(0)
    assert result.GetNumberOfPoints() == 12
    assert result.GetNumberOfCells() == 4
    np.testing.assert_allclose(
        result.GetField("pointVar"),
        np.array([10.1, 20.1, 30.1, 40.1, 50.1, 60.1, 10.1, 20.1, 30.1, 40.1, 50.1, 60.1], dtype=np.float32),
    )
    np.testing.assert_allclose(
        result.GetField("cellVar"),
        np.array([100.1, 200.1, 100.1, 200.1], dtype=np.float32),
    )
    np.testing.assert_allclose(
        result.GetCoordinateSystem().GetData().AsNumPy(),
        np.array(
            [
                [0, 0, 0],
                [1, 0, 0],
                [2, 0, 0],
                [0, 1, 0],
                [1, 1, 0],
                [2, 1, 0],
                [3, 0, 0],
                [4, 0, 0],
                [5, 0, 0],
                [3, 1, 0],
                [4, 1, 0],
                [5, 1, 0],
            ],
            dtype=np.float32,
        ),
    )


def test_triangle_same_fields_same_data_type():
    triangulate = Triangulate()
    input_partitions = PartitionedDataSet()

    dataset0 = create_uniform_data((0.0, 0.0))
    dataset1 = create_uniform_data((3.0, 0.0))
    input_partitions.AppendPartition(triangulate.Execute(dataset0))
    input_partitions.AppendPartition(triangulate.Execute(dataset1))

    result = MergeDataSets().Execute(input_partitions).GetPartition(0)
    assert result.GetNumberOfPoints() == 12
    assert result.GetNumberOfCells() == 8
    np.testing.assert_allclose(
        result.GetField("pointVar"),
        np.array([10.1, 20.1, 30.1, 40.1, 50.1, 60.1, 10.1, 20.1, 30.1, 40.1, 50.1, 60.1], dtype=np.float32),
    )
    np.testing.assert_allclose(
        result.GetField("cellVar"),
        np.array([100.1, 100.1, 200.1, 200.1, 100.1, 100.1, 200.1, 200.1], dtype=np.float32),
    )


def test_missing_fields_and_same_field_name():
    data_set1 = create_uniform_dataset((3, 2), origin=(0.0, 0.0), spacing=(1, 1))
    data_set2 = create_uniform_dataset((3, 2), origin=(0.0, 0.0), spacing=(1, 1))

    point_var = np.array([10.1, 20.1, 30.1, 40.1, 50.1, 60.1], dtype=np.float32)
    cell_var = np.array([100, 200], dtype=np.int64)
    data_set1.AddPointField("pointVar", point_var)
    data_set2.AddCellField("cellVar", cell_var)
    data_set1.AddPointField("fieldSameName", point_var)
    data_set2.AddCellField("fieldSameName", cell_var)
    data_set1.AddPointField("fieldSameName2", point_var)
    data_set2.AddPointField("fieldSameName2", point_var)
    data_set2.AddCellField("fieldSameName2", cell_var)

    input_data_sets = PartitionedDataSet()
    input_data_sets.AppendPartition(data_set1)
    input_data_sets.AppendPartition(data_set2)

    merge = MergeDataSets()
    merge.SetInvalidValue(0.0)
    result = merge.Execute(input_data_sets).GetPartition(0)

    np.testing.assert_allclose(
        result.GetField("pointVar", Association.POINTS),
        np.array([10.1, 20.1, 30.1, 40.1, 50.1, 60.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float32),
    )
    np.testing.assert_array_equal(
        result.GetField("cellVar", Association.CELLS),
        np.array([0, 0, 100, 200], dtype=np.int64),
    )
    np.testing.assert_allclose(
        result.GetField("fieldSameName", Association.POINTS),
        np.array([10.1, 20.1, 30.1, 40.1, 50.1, 60.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float32),
    )
    np.testing.assert_array_equal(
        result.GetField("fieldSameName", Association.CELLS),
        np.array([0, 0, 100, 200], dtype=np.int64),
    )
    np.testing.assert_allclose(
        result.GetField("fieldSameName2", Association.POINTS),
        np.array([10.1, 20.1, 30.1, 40.1, 50.1, 60.1, 10.1, 20.1, 30.1, 40.1, 50.1, 60.1], dtype=np.float32),
    )
    np.testing.assert_array_equal(
        result.GetField("fieldSameName2", Association.CELLS),
        np.array([0, 0, 100, 200], dtype=np.int64),
    )


def test_different_coords():
    data_set0 = create_uniform_data((0.0, 0.0))
    extra_coords = np.zeros((6, 3), dtype=np.float32)
    data_set0.AddCoordinateSystem("coordsExtra", extra_coords)
    data_set1 = create_uniform_data((3.0, 0.0))

    input_data_sets = PartitionedDataSet()
    input_data_sets.AppendPartition(data_set0)
    input_data_sets.AppendPartition(data_set1)
    try:
        MergeDataSets().Execute(input_data_sets)
    except RuntimeError as error:
        assert "different number of coordinate systems" in str(error)

    data_set2 = create_uniform_dataset((3, 2), origin=(0.0, 0.0), spacing=(1, 1))
    data_set2.AddPointField("pointVarExtra", np.array([10.1, 20.1, 30.1, 40.1, 50.1, 60.1], dtype=np.float32))
    data_set2.AddCellField("cellVarExtra", np.array([100.1, 200.1], dtype=np.float32))

    input_data_sets2 = PartitionedDataSet()
    input_data_sets2.AppendPartition(data_set1)
    input_data_sets2.AppendPartition(data_set2)
    try:
        MergeDataSets().Execute(input_data_sets2)
    except RuntimeError as error:
        assert "Coordinates system name:" in str(error)


def main():
    test_uniform_same_fields_same_data_type()
    test_triangle_same_fields_same_data_type()
    test_missing_fields_and_same_field_name()
    test_different_coords()


if __name__ == "__main__":
    main()
