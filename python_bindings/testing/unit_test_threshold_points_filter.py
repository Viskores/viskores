##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.filter.entity_extraction import ThresholdPoints
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    dataset = maker.Make2DUniformDataSet1()
    threshold_points = ThresholdPoints()
    threshold_points.SetThresholdBetween(40.0, 71.0)
    threshold_points.SetActiveField("pointvar")
    threshold_points.SetFieldsToPass("pointvar")
    output = threshold_points.Execute(dataset)
    assert output.GetNumberOfCells() == 11
    point_field = output.GetField("pointvar")
    assert point_field.shape[0] == 25
    assert point_field[12] == 50.0

    dataset = maker.Make3DUniformDataSet1()
    threshold_points = ThresholdPoints()
    threshold_points.SetThresholdAbove(1.0)
    threshold_points.SetCompactPoints(True)
    threshold_points.SetActiveField("pointvar")
    threshold_points.SetFieldsToPass("pointvar")
    output = threshold_points.Execute(dataset)
    assert output.GetNumberOfCells() == 27
    point_field = output.GetField("pointvar")
    assert point_field.shape[0] == 27
    assert point_field[0] == 99.0

    dataset = maker.Make3DExplicitDataSet5()
    threshold_points = ThresholdPoints()
    threshold_points.SetThresholdBelow(50.0)
    threshold_points.SetCompactPoints(True)
    threshold_points.SetActiveField("pointvar")
    threshold_points.SetFieldsToPass("pointvar")
    output = threshold_points.Execute(dataset)
    assert output.GetNumberOfCells() == 6
    point_field = output.GetField("pointvar")
    assert point_field.shape[0] == 6
    assert point_field[4] == 10.0

    dataset = maker.Make3DExplicitDataSet1()
    threshold_points = ThresholdPoints()
    threshold_points.SetThresholdBetween(500.0, 600.0)
    threshold_points.SetActiveField("pointvar")
    threshold_points.SetFieldsToPass("pointvar")
    output = threshold_points.Execute(dataset)
    assert len(output.FieldNames()) == 2
    assert output.GetNumberOfCells() == 0


if __name__ == "__main__":
    main()
