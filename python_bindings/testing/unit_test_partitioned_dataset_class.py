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
from viskores.cont import Association, Field, PartitionedDataSet, create_uniform_dataset


def make_partition(offset):
    dataset = create_uniform_dataset((4,))
    dataset.AddPointField("point_values", np.arange(4, dtype=np.float32) + offset)
    dataset.AddCellField("cell_values", np.arange(3, dtype=np.int32) + int(offset))
    return dataset


def expect_raises(callable_object, *args):
    try:
        callable_object(*args)
    except Exception:
        return
    raise AssertionError("Expected an exception.")


def test_partitions():
    partitions = PartitionedDataSet()
    assert partitions.GetNumberOfPartitions() == 0

    first = make_partition(0.0)
    second = make_partition(10.0)
    partitions.AppendPartition(first)
    partitions.AppendPartition(second)

    assert partitions.GetNumberOfPartitions() == 2
    assert partitions.GetPartition(0).GetNumberOfPoints() == 4
    assert partitions.GetPartition(0).GetNumberOfCells() == 3
    assert partitions.GetPartition(1).GetNumberOfPoints() == 4
    np.testing.assert_array_equal(
        partitions.GetPartition(0).GetField("point_values"),
        np.asarray([0.0, 1.0, 2.0, 3.0], dtype=np.float32),
    )
    np.testing.assert_array_equal(
        partitions.GetPartition(1).GetField("point_values"),
        np.asarray([10.0, 11.0, 12.0, 13.0], dtype=np.float32),
    )


def test_partition_fields():
    partitions = PartitionedDataSet()
    partitions.AppendPartition(make_partition(0.0))
    partitions.AppendPartition(make_partition(10.0))

    partition_values = np.asarray([100, 200], dtype=np.int32)
    partitions.AddField(Field("partition_values", Association.PARTITIONS, partition_values))

    assert partitions.GetNumberOfFields() == 1
    assert partitions.HasField("partition_values")
    assert partitions.HasField("partition_values", Association.PARTITIONS)
    assert not partitions.HasField("partition_values", Association.POINTS)
    np.testing.assert_array_equal(partitions.GetField("partition_values"), partition_values)
    np.testing.assert_array_equal(partitions.GetField("partition_values", "partitions"), partition_values)

    expect_raises(partitions.GetField, "partition_values", Association.POINTS)
    expect_raises(partitions.GetField, "missing")


def main():
    viskores.cont.Initialize(["unit_test_partitioned_dataset_class.py"])
    test_partitions()
    test_partition_fields()


if __name__ == "__main__":
    main()
