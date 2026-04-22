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

import viskores.cont
from viskores.filter.density_estimate import Entropy
from viskores.source import Tangle


def main():
    viskores.cont.Initialize(["unit_test_entropy_filter.py"])

    tangle = Tangle()
    tangle.SetCellDimensions((32, 32, 32))
    dataset = tangle.Execute()

    entropy = Entropy()
    entropy.SetNumberOfBins(50)
    entropy.SetActiveField("tangle")
    result = entropy.Execute(dataset)

    entropy_value = float(result.GetField("entropy")[0])
    assert math.fabs(entropy_value - 4.59093) < 0.001 or math.fabs(entropy_value - 4.59798) < 0.001


if __name__ == "__main__":
    main()

