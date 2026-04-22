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

from viskores.filter.clean_grid import CleanGrid
from viskores.filter.entity_extraction import Threshold
from viskores.testing import MakeTestDataSet


def check_regular_2d(return_all_in_range):
    dataset = MakeTestDataSet().Make2DUniformDataSet0()
    threshold = Threshold()
    if return_all_in_range:
        threshold.SetLowerThreshold(10.0)
        threshold.SetUpperThreshold(60.0)
    else:
        threshold.SetLowerThreshold(60.0)
        threshold.SetUpperThreshold(61.0)
    threshold.SetAllInRange(return_all_in_range)
    threshold.SetActiveField("pointvar")
    threshold.SetFieldsToPass("cellvar")
    output = threshold.Execute(dataset)
    assert len(output.FieldNames()) == 2
    cell_field = output.GetField("cellvar")
    if return_all_in_range:
        assert np.array_equal(cell_field, np.array([100.1], dtype=cell_field.dtype))
    else:
        assert np.array_equal(cell_field, np.array([200.1], dtype=cell_field.dtype))
    CleanGrid().Execute(output)


def check_regular_3d(return_all_in_range):
    dataset = MakeTestDataSet().Make3DUniformDataSet0()
    threshold = Threshold()
    if return_all_in_range:
        threshold.SetLowerThreshold(10.1)
        threshold.SetUpperThreshold(180.0)
    else:
        threshold.SetLowerThreshold(20.0)
        threshold.SetUpperThreshold(21.0)
    threshold.SetAllInRange(return_all_in_range)
    threshold.SetActiveField("pointvar")
    threshold.SetFieldsToPass("cellvar")
    output = threshold.Execute(dataset)
    assert len(output.FieldNames()) == 2
    cell_field = output.GetField("cellvar")
    if return_all_in_range:
        expected = np.array([100.1, 100.2, 100.3], dtype=cell_field.dtype)
    else:
        expected = np.array([100.1, 100.2], dtype=cell_field.dtype)
    assert np.array_equal(cell_field, expected)
    CleanGrid().Execute(output)


def main():
    check_regular_2d(True)
    check_regular_2d(False)
    check_regular_3d(True)
    check_regular_3d(False)

    dataset = MakeTestDataSet().Make3DExplicitDataSet1()
    threshold = Threshold()
    threshold.SetLowerThreshold(20.0)
    threshold.SetUpperThreshold(21.0)
    threshold.SetActiveField("pointvar")
    threshold.SetFieldsToPass("cellvar")
    output = threshold.Execute(dataset)
    assert len(output.FieldNames()) == 2
    assert np.array_equal(output.GetField("cellvar"), np.array([100.1, 100.2], dtype=np.float32))
    CleanGrid().Execute(output)

    threshold = Threshold()
    threshold.SetLowerThreshold(500.0)
    threshold.SetUpperThreshold(500.1)
    threshold.SetActiveField("pointvar")
    threshold.SetFieldsToPass("cellvar")
    output = threshold.Execute(dataset)
    assert len(output.FieldNames()) == 2
    assert output.GetField("cellvar").shape[0] == 0
    CleanGrid().Execute(output)


if __name__ == "__main__":
    main()
