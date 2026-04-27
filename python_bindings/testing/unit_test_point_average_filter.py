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
from viskores.filter.field_conversion import PointAverage


def main():
    dataset = make_3d_uniform_dataset0()

    point_average = PointAverage()
    point_average.SetOutputFieldName("avgvals")
    point_average.SetActiveField("cellvar")
    result = point_average.Execute(dataset)

    expected = np.array(
        [
            100.1,
            100.15,
            100.2,
            100.1,
            100.15,
            100.2,
            100.2,
            100.25,
            100.3,
            100.2,
            100.25,
            100.3,
            100.3,
            100.35,
            100.4,
            100.3,
            100.35,
            100.4,
        ],
        dtype=np.float64,
    )
    assert np.allclose(result.GetField("avgvals"), expected, rtol=1e-6, atol=1e-6)


if __name__ == "__main__":
    main()
