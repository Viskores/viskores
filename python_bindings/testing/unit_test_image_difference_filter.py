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
from viskores.filter.image_processing import ImageDifference


def fill_dataset(fudge_factor):
    dataset = create_uniform_dataset((5, 5))
    primary = np.ones((25, 4), dtype=np.float32)
    secondary = np.ones((25, 4), dtype=np.float32)
    secondary[:, 0] = fudge_factor
    dataset.AddPointField("primary", primary)
    dataset.AddPointField("secondary", secondary)
    return dataset


def check_result(result, expected_diff_value, expected_threshold_value):
    assert result.HasField("image-diff", association="points")
    assert result.HasField("threshold-output", association="points")
    np.testing.assert_allclose(
        result.GetField("image-diff"),
        np.tile(np.array([expected_diff_value, 0.0, 0.0, 0.0], dtype=np.float32), (25, 1)),
    )
    np.testing.assert_allclose(
        result.GetField("threshold-output"),
        np.full(25, expected_threshold_value, dtype=np.float64),
    )


def main():
    viskores.cont.Initialize(["unit_test_image_difference_filter.py"])

    filter_obj = ImageDifference()
    filter_obj.SetPrimaryField("primary")
    filter_obj.SetSecondaryField("secondary")
    filter_obj.SetPixelDiffThreshold(0.05)
    filter_obj.SetPixelShiftRadius(0)
    result = filter_obj.Execute(fill_dataset(1.0))
    check_result(result, 0.0, 0.0)
    assert filter_obj.GetImageDiffWithinThreshold()

    filter_obj = ImageDifference()
    filter_obj.SetPrimaryField("primary")
    filter_obj.SetSecondaryField("secondary")
    filter_obj.SetPixelDiffThreshold(0.05)
    filter_obj.SetPixelShiftRadius(1)
    filter_obj.SetAverageRadius(1)
    result = filter_obj.Execute(fill_dataset(1.0))
    check_result(result, 0.0, 0.0)
    assert filter_obj.GetImageDiffWithinThreshold()

    filter_obj = ImageDifference()
    filter_obj.SetPrimaryField("primary")
    filter_obj.SetSecondaryField("secondary")
    filter_obj.SetPixelDiffThreshold(0.05)
    filter_obj.SetPixelShiftRadius(0)
    result = filter_obj.Execute(fill_dataset(3.0))
    check_result(result, 2.0, 2.0)
    assert not filter_obj.GetImageDiffWithinThreshold()

    filter_obj = ImageDifference()
    filter_obj.SetPrimaryField("primary")
    filter_obj.SetSecondaryField("secondary")
    filter_obj.SetPixelDiffThreshold(0.05)
    filter_obj.SetPixelShiftRadius(0)
    filter_obj.SetAllowedPixelErrorRatio(1.0)
    result = filter_obj.Execute(fill_dataset(3.0))
    check_result(result, 2.0, 2.0)
    assert filter_obj.GetImageDiffWithinThreshold()

    filter_obj = ImageDifference()
    filter_obj.SetPrimaryField("primary")
    filter_obj.SetSecondaryField("secondary")
    filter_obj.SetPixelDiffThreshold(3.0)
    filter_obj.SetPixelShiftRadius(0)
    result = filter_obj.Execute(fill_dataset(3.0))
    check_result(result, 2.0, 2.0)
    assert filter_obj.GetImageDiffWithinThreshold()


if __name__ == "__main__":
    main()
