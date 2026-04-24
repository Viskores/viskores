##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import math
import numpy as np

import viskores.cont
from viskores.cont import PartitionedDataSet, create_uniform_dataset
from viskores.filter.density_estimate import Statistics


def get_stat(data_object, name):
    return float(data_object.GetField(name)[0])


def make_dataset(values):
    if len(values) == 0:
        dataset = viskores.cont.DataSet()
        dataset.AddPointField("scalarField", np.asarray(values, dtype=np.float32))
        return dataset
    dataset = create_uniform_dataset((len(values),))
    dataset.AddPointField("scalarField", np.asarray(values, dtype=np.float32))
    return dataset


def check_stats(data_object):
    assert math.isclose(get_stat(data_object, "N"), 1000.0, rel_tol=1e-6)
    assert math.isclose(get_stat(data_object, "Min"), 0.0, rel_tol=1e-6)
    assert math.isclose(get_stat(data_object, "Max"), 999.0, rel_tol=1e-6)
    assert math.isclose(get_stat(data_object, "Sum"), 499500.0, rel_tol=1e-6)
    assert math.isclose(get_stat(data_object, "Mean"), 499.5, rel_tol=1e-6)
    assert math.isclose(get_stat(data_object, "SampleVariance"), 83416.66, rel_tol=1e-4, abs_tol=1e-2)
    assert math.isclose(get_stat(data_object, "SampleStddev"), 288.819, rel_tol=1e-4, abs_tol=1e-3)
    assert math.isclose(get_stat(data_object, "Skewness"), 0.0, abs_tol=1e-6)
    assert math.isclose(get_stat(data_object, "Kurtosis"), 1.8, rel_tol=1e-4, abs_tol=1e-4)
    assert math.isclose(get_stat(data_object, "PopulationStddev"), 288.675, rel_tol=1e-4, abs_tol=1e-3)
    assert math.isclose(get_stat(data_object, "PopulationVariance"), 83333.3, rel_tol=1e-4, abs_tol=1e-2)


def main():
    viskores.cont.Initialize(["unit_test_statistics_filter.py"])

    statistics = Statistics()
    statistics.SetActiveField("scalarField")

    dataset = make_dataset(np.arange(1000, dtype=np.float32))
    single_result = statistics.Execute(dataset)
    check_stats(single_result)

    partitions = PartitionedDataSet()
    for start in range(0, 1000, 100):
        partitions.AppendPartition(make_dataset(np.arange(start, start + 100, dtype=np.float32)))
    partitions.AppendPartition(make_dataset(np.array([], dtype=np.float32)))

    partitioned_result = statistics.Execute(partitions)
    check_stats(partitioned_result)
    assert partitioned_result.GetNumberOfPartitions() == partitions.GetNumberOfPartitions()

    for partition_id in range(partitioned_result.GetNumberOfPartitions()):
        expected = statistics.Execute(partitions.GetPartition(partition_id))
        actual = partitioned_result.GetPartition(partition_id)
        for field_name in (
            "N",
            "Min",
            "Max",
            "Sum",
            "Mean",
            "SampleVariance",
            "SampleStddev",
            "Skewness",
            "Kurtosis",
            "PopulationStddev",
            "PopulationVariance",
        ):
            np.testing.assert_allclose(actual.GetField(field_name), expected.GetField(field_name))


if __name__ == "__main__":
    main()
