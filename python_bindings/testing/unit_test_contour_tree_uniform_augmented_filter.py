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
from viskores.filter.scalar_topology import ContourTreeAugmented
from viskores.testing import MakeTestDataSet


def main():
    viskores.cont.Initialize(["unit_test_contour_tree_uniform_augmented_filter.py"])

    maker = MakeTestDataSet()

    augmented2d = ContourTreeAugmented(False, 1)
    augmented2d.SetActiveField("pointvar")
    result2d = augmented2d.Execute(maker.Make2DUniformDataSet1())
    assert result2d.GetNumberOfPoints() == 25
    expected_2d = np.array(
        [[0, 12], [4, 13], [12, 13], [12, 18], [12, 20], [13, 14], [13, 19]], dtype=np.int64
    )
    saddle_peak_2d = augmented2d.GetSortedSuperarcs()
    assert saddle_peak_2d.shape == expected_2d.shape
    assert np.array_equal(saddle_peak_2d, expected_2d)

    augmented3d = ContourTreeAugmented(False, 1)
    augmented3d.SetActiveField("pointvar")
    result3d = augmented3d.Execute(maker.Make3DUniformDataSet1())
    assert result3d.GetNumberOfPoints() == 125
    expected_3d = np.array(
        [
            [0, 67],
            [31, 42],
            [42, 43],
            [42, 56],
            [56, 67],
            [56, 92],
            [62, 67],
            [81, 92],
            [92, 93],
        ],
        dtype=np.int64,
    )
    saddle_peak_3d = augmented3d.GetSortedSuperarcs()
    assert saddle_peak_3d.shape == expected_3d.shape
    assert np.array_equal(saddle_peak_3d, expected_3d)
    assert int(augmented3d.GetNumIterations()) > 0


if __name__ == "__main__":
    main()
