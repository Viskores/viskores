##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.filter.entity_extraction import MaskPoints
from viskores.testing import MakeTestDataSet


def test_regular_2d():
    dataset = MakeTestDataSet().Make2DUniformDataSet1()
    mask_points = MaskPoints()
    mask_points.SetStride(2)
    mask_points.SetFieldsToPass("pointvar")
    output = mask_points.Execute(dataset)

    assert output.GetNumberOfCells() == 12
    assert output.GetField("pointvar").shape[0] == 12


def test_regular_3d():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    mask_points = MaskPoints()
    mask_points.SetStride(5)
    mask_points.SetFieldsToPass("pointvar")
    output = mask_points.Execute(dataset)

    assert output.GetNumberOfCells() == 25
    assert output.GetField("pointvar").shape[0] == 25


def test_explicit_3d():
    dataset = MakeTestDataSet().Make3DExplicitDataSet5()
    mask_points = MaskPoints()
    mask_points.SetStride(3)
    mask_points.SetCompactPoints(False)
    mask_points.SetFieldsToPass("pointvar")
    output = mask_points.Execute(dataset)

    assert output.GetNumberOfCells() == 3
    assert output.GetField("pointvar").shape[0] == 11


def main():
    test_regular_2d()
    test_regular_3d()
    test_explicit_3d()


if __name__ == "__main__":
    main()
