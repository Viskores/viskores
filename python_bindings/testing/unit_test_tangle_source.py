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

from viskores.source import Tangle


def main():
    source = Tangle()
    source.SetCellDimensions((20, 20, 20))
    dataset = source.Execute()

    assert dataset.GetNumberOfPoints() == 9261
    assert dataset.GetNumberOfCells() == 8000

    scalars = dataset.GetField("tangle")
    assert scalars.shape == (9261,)

    expected = {
        0: 24.46,
        16: 16.1195,
        21: 20.5988,
        256: 8.58544,
        1024: 1.56976,
        1987: 1.04074,
        2048: 0.95236,
        3110: 6.39556,
        4097: 2.62186,
        6599: 7.79722,
        7999: 7.94986,
    }
    for index, value in expected.items():
        assert np.isclose(scalars[index], value, rtol=1e-4, atol=1e-4)


if __name__ == "__main__":
    main()
