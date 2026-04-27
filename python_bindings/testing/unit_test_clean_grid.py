##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.filter.clean_grid import CleanGrid
from viskores.filter.contour import ContourMarchingCells
from viskores.testing import MakeTestDataSet


def test_uniform_grid(compact_point_fields, merge_points):
    clean = CleanGrid()
    clean.SetCompactPointFields(compact_point_fields)
    clean.SetMergePoints(merge_points)
    clean.SetFieldsToPass(("pointvar", "cellvar"))

    output = clean.Execute(MakeTestDataSet().Make2DUniformDataSet0())
    assert output.HasField("pointvar")
    assert output.HasField("cellvar")
    assert output.GetNumberOfPoints() == 6
    assert output.GetNumberOfCells() == 2

    point_field = output.GetField("pointvar")
    assert point_field.shape[0] == 6
    assert point_field[1] == 20.1
    assert point_field[4] == 50.1

    cell_field = output.GetField("cellvar")
    assert cell_field.shape[0] == 2
    assert cell_field[0] == 100.1
    assert cell_field[1] == 200.1


def test_point_merging():
    base_data = MakeTestDataSet().Make3DUniformDataSet3((4, 4, 4))
    contour = ContourMarchingCells()
    contour.SetIsoValue(0.05)
    contour.SetActiveField("pointvar")
    contour.SetMergeDuplicatePoints(False)
    in_data = contour.Execute(base_data)

    assert in_data.GetNumberOfCells() == 76
    assert in_data.GetNumberOfPoints() == 228

    clean = CleanGrid()
    clean.SetCompactPointFields(False)
    clean.SetMergePoints(False)
    clean.SetRemoveDegenerateCells(False)
    no_merging = clean.Execute(in_data)
    assert no_merging.GetNumberOfCells() == 76
    assert no_merging.GetNumberOfPoints() == 228
    assert no_merging.GetField("pointvar").shape[0] == 228
    assert no_merging.GetField("cellvar").shape[0] == 76

    clean.SetMergePoints(True)
    clean.SetFastMerge(False)
    close_merge = clean.Execute(in_data)
    assert close_merge.GetNumberOfCells() == 76
    assert close_merge.GetNumberOfPoints() == 62
    assert close_merge.GetField("pointvar").shape[0] == 62
    assert close_merge.GetField("cellvar").shape[0] == 76

    clean.SetFastMerge(True)
    close_fast_merge = clean.Execute(in_data)
    assert close_fast_merge.GetNumberOfCells() == 76
    assert close_fast_merge.GetNumberOfPoints() == 62

    clean.SetFastMerge(False)
    clean.SetTolerance(0.1)
    far_merge = clean.Execute(in_data)
    assert far_merge.GetNumberOfCells() == 76
    assert far_merge.GetNumberOfPoints() == 36

    clean.SetFastMerge(True)
    far_fast_merge = clean.Execute(in_data)
    assert far_fast_merge.GetNumberOfCells() == 76
    assert far_fast_merge.GetNumberOfPoints() == 19

    clean.SetRemoveDegenerateCells(True)
    no_degenerate_cells = clean.Execute(in_data)
    assert no_degenerate_cells.GetNumberOfCells() == 18
    assert no_degenerate_cells.GetNumberOfPoints() == 19
    assert no_degenerate_cells.GetField("cellvar").shape[0] == 18


def main():
    test_uniform_grid(True, False)
    test_uniform_grid(False, False)
    test_uniform_grid(True, True)
    test_uniform_grid(False, True)
    test_point_merging()


if __name__ == "__main__":
    main()
