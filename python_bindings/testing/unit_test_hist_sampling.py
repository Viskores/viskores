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
from viskores.filter.entity_extraction import ThresholdPoints
from viskores.filter.resampling import HistSampling


def create_scalar_field(size_per_dim):
    x, y, z = np.indices((size_per_dim, size_per_dim, size_per_dim), dtype=np.float32)
    center = np.float32(size_per_dim / 2.0)
    radius2 = (x - center) ** 2 + (y - center) ** 2 + (z - center) ** 2
    result = np.empty_like(radius2, dtype=np.float32)
    mask = radius2 < np.float32(0.5)
    result[mask] = np.float32(10.0)
    result[~mask] = np.float32(10.0) / np.sqrt(radius2[~mask])
    return result.reshape(-1)


def main():
    viskores.cont.Initialize(["unit_test_hist_sampling.py"])

    size_per_dim = 20
    dataset = create_uniform_dataset((size_per_dim, size_per_dim, size_per_dim))
    dataset.AddPointField("scalarField", create_scalar_field(size_per_dim))

    hist_sampling = HistSampling()
    hist_sampling.SetNumberOfBins(10)
    hist_sampling.SetActiveField("scalarField")
    output = hist_sampling.Execute(dataset)

    threshold = ThresholdPoints()
    threshold.SetActiveField("scalarField")
    threshold.SetCompactPoints(True)
    threshold.SetThresholdAbove(9.9)
    threshold_output = threshold.Execute(output)

    assert threshold_output.GetField("scalarField").shape[0] == 7


if __name__ == "__main__":
    main()
