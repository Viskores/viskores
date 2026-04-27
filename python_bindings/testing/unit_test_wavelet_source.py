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

from viskores.source import Wavelet


def main():
    source = Wavelet()
    dataset = source.Execute()

    assert dataset.GetNumberOfPoints() == 9261
    assert dataset.GetNumberOfCells() == 8000

    scalars = dataset.GetField("RTData")
    assert scalars.shape == (9261,)

    expected = {
        0: 60.7635,
        16: 99.6115,
        21: 69.1968,
        256: 118.620,
        1024: 140.466,
        1987: 203.720,
        2048: 223.010,
        3110: 128.282,
        4097: 153.913,
        6599: 120.068,
        7999: 65.6710,
    }
    for index, value in expected.items():
        assert np.isclose(scalars[index], value, rtol=1e-4, atol=1e-4)


if __name__ == "__main__":
    main()
