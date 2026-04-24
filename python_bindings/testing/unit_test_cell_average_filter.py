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

from python_test_data import make_3d_uniform_dataset0
from viskores.filter.field_conversion import CellAverage


def main():
    dataset = make_3d_uniform_dataset0()

    cell_average = CellAverage()
    cell_average.SetOutputFieldName("avgvals")
    cell_average.SetActiveField("pointvar")
    result = cell_average.Execute(dataset)

    expected = np.array([60.1875, 70.2125, 120.337494, 130.3625], dtype=np.float64)
    assert np.allclose(result.GetField("avgvals"), expected, rtol=1e-6, atol=1e-6)

    cell_average.SetOutputFieldName("avgpos")
    cell_average.SetUseCoordinateSystemAsField(True)
    result = cell_average.Execute(dataset)

    expected_positions = np.array(
        [
            [0.5, 0.5, 0.5],
            [1.5, 0.5, 0.5],
            [0.5, 0.5, 1.5],
            [1.5, 0.5, 1.5],
        ],
        dtype=np.float64,
    )
    assert np.allclose(result.GetField("avgpos"), expected_positions, rtol=1e-6, atol=1e-6)


if __name__ == "__main__":
    main()
