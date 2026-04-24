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
from viskores import Range
from viskores.cont import create_uniform_dataset
from viskores.filter.density_estimate import Histogram


def main():
    viskores.cont.Initialize(["unit_test_histogram_filter.py"])

    dataset = create_uniform_dataset((10,))
    dataset.AddPointField("values", np.arange(10, dtype=np.float32))

    histogram = Histogram()
    histogram.SetActiveField("values")
    histogram.SetNumberOfBins(5)
    histogram.SetRange(Range(0.0, 10.0))
    result = histogram.Execute(dataset)

    np.testing.assert_array_equal(result.GetField("histogram"), np.array([2, 2, 2, 2, 2], dtype=np.int64))
    assert histogram.GetRange().Min == 0.0
    assert histogram.GetRange().Max == 10.0
    assert histogram.GetComputedRange().Min == 0.0
    assert histogram.GetComputedRange().Max == 10.0
    assert histogram.GetBinDelta() == 2.0


if __name__ == "__main__":
    main()

