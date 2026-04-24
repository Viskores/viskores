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
from viskores.cont import create_uniform_dataset
from viskores.filter.density_estimate import NDEntropy


def main():
    viskores.cont.Initialize(["unit_test_nd_entropy_filter.py"])

    dataset = create_uniform_dataset((8,))
    dataset.AddPointField("fieldA", np.array([0, 0, 0, 0, 1, 1, 1, 1], dtype=np.float32))
    dataset.AddPointField("fieldB", np.array([0, 0, 1, 1, 0, 0, 1, 1], dtype=np.float32))
    dataset.AddPointField("fieldC", np.array([0, 1, 0, 1, 0, 1, 0, 1], dtype=np.float32))

    entropy = NDEntropy()
    entropy.AddFieldAndBin("fieldA", 2)
    entropy.AddFieldAndBin("fieldB", 2)
    entropy.AddFieldAndBin("fieldC", 2)
    result = entropy.Execute(dataset)

    entropy_value = float(result.GetField("Entropy")[0])
    assert math.isclose(entropy_value, 3.0, rel_tol=1e-6, abs_tol=1e-6)


if __name__ == "__main__":
    main()
