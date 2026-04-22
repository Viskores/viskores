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
from viskores.cont import create_uniform_dataset
from viskores.filter.density_estimate import NDHistogram


def main():
    viskores.cont.Initialize(["unit_test_nd_histogram_filter.py"])

    dataset = create_uniform_dataset((8,))
    dataset.AddPointField("fieldA", np.array([0, 0, 0, 0, 1, 1, 1, 1], dtype=np.float32))
    dataset.AddPointField("fieldB", np.array([0, 0, 1, 1, 0, 0, 1, 1], dtype=np.float32))
    dataset.AddPointField("fieldC", np.array([0, 1, 0, 1, 0, 1, 0, 1], dtype=np.float32))

    histogram = NDHistogram()
    histogram.AddFieldAndBin("fieldA", 2)
    histogram.AddFieldAndBin("fieldB", 2)
    histogram.AddFieldAndBin("fieldC", 2)
    output = histogram.Execute(dataset)

    expected = np.array(
        [
            [0, 0, 0, 1],
            [0, 0, 1, 1],
            [0, 1, 0, 1],
            [0, 1, 1, 1],
            [1, 0, 0, 1],
            [1, 0, 1, 1],
            [1, 1, 0, 1],
            [1, 1, 1, 1],
        ],
        dtype=np.int64,
    )
    actual = np.column_stack(
        [
            output.GetField("fieldA"),
            output.GetField("fieldB"),
            output.GetField("fieldC"),
            output.GetField("Frequency"),
        ]
    )
    np.testing.assert_array_equal(actual, expected)
    assert histogram.GetBinDelta(0) > 0.0
    assert histogram.GetDataRange(0).Max >= histogram.GetDataRange(0).Min


if __name__ == "__main__":
    main()
