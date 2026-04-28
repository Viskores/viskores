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
from viskores.filter.scalar_topology import ContourTreeAugmented, helper
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
    supernodes_2d = augmented2d.GetSupernodes()
    superarcs_2d = augmented2d.GetSuperarcs()
    intrinsic_volume_2d = augmented2d.GetIntrinsicVolume()
    dependent_volume_2d = augmented2d.GetDependentVolume()
    point_superarc_ids_2d = augmented2d.GetPointSuperarcIds()
    superarc_point_ids_2d = helper.group_points_by_superarc(point_superarc_ids_2d)
    supernode_point_ids_2d = augmented2d.GetSupernodePointIds()

    assert not hasattr(augmented2d, "GetSuperarcPointOffsets")
    assert not hasattr(augmented2d, "GetSuperarcPointIds")
    assert supernodes_2d.ndim == 1
    assert superarcs_2d.shape == supernodes_2d.shape
    assert intrinsic_volume_2d.shape == supernodes_2d.shape
    assert dependent_volume_2d.shape == supernodes_2d.shape
    assert point_superarc_ids_2d.shape == (result2d.GetNumberOfPoints(),)
    assert superarc_point_ids_2d.GetNumberOfValues() == superarcs_2d.shape[0]
    assert superarc_point_ids_2d.GetNumberOfComponentsFlat() == 0
    assert supernode_point_ids_2d.shape == supernodes_2d.shape
    superarc_point_groups_2d = superarc_point_ids_2d.AsList()
    assert len(superarc_point_groups_2d) == superarcs_2d.shape[0]
    all_superarc_point_ids_2d = superarc_point_ids_2d.GetComponentsArray().AsNumPy()
    assert np.array_equal(
        np.sort(all_superarc_point_ids_2d), np.arange(result2d.GetNumberOfPoints())
    )
    for superarc_id, point_ids in enumerate(superarc_point_groups_2d):
        assert int(intrinsic_volume_2d[superarc_id]) == point_ids.shape[0]
        assert np.all(point_superarc_ids_2d[point_ids] == superarc_id)

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
