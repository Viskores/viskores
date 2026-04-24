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
from viskores.cont import PartitionedDataSet, create_uniform_dataset
from viskores.filter.density_estimate import Histogram


def make_partition(values):
    dataset = create_uniform_dataset((len(values),))
    dataset.AddPointField("double", np.asarray(values, dtype=np.float64))
    return dataset


def main():
    viskores.cont.Initialize(["unit_test_partitioned_dataset_histogram_filter.py"])

    partitions = PartitionedDataSet()
    partitions.AppendPartition(make_partition(np.linspace(0.0, 100.0, 1024, dtype=np.float64)))
    partitions.AppendPartition(make_partition(np.linspace(100.0, 1000.0, 1024, dtype=np.float64)))
    partitions.AppendPartition(make_partition(np.linspace(100.0, 500.0, 1024, dtype=np.float64)))

    histogram = Histogram()
    histogram.SetActiveField("double")
    result = histogram.Execute(partitions)

    assert result.GetNumberOfPartitions() == 1
    bins = result.GetPartition(0).GetField("histogram")
    assert bins.shape[0] == 10
    assert int(np.sum(bins)) == 1024 * 3


if __name__ == "__main__":
    main()

