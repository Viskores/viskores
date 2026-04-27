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

from viskores.source import Oscillator


def main():
    source = Oscillator()
    source.SetPointDimensions((21, 21, 21))
    source.SetTime(0.5)
    source.AddDamped(0.25, 0.25, 0.25, 0.5, 0.1, 0.2)
    source.AddDecaying(0.5, 0.5, 0.5, 0.35, 0.2, 0.1)
    source.AddPeriodic(0.6, 0.2, 0.7, 0.15, 0.1, 0.2)

    dataset = source.Execute()

    assert dataset.GetNumberOfPoints() == 9261
    assert dataset.GetNumberOfCells() == 8000

    scalars = dataset.GetField("oscillating")
    assert scalars.shape == (9261,)

    expected = {
        0: -0.0163996,
        16: -0.0182232,
        21: -0.0181952,
        3110: -0.0404135,
    }
    for index, value in expected.items():
        assert np.isclose(scalars[index], value, rtol=1e-5, atol=1e-5)


if __name__ == "__main__":
    main()
