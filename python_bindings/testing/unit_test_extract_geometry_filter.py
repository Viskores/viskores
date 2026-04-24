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

from viskores import Box
from viskores.filter.clean_grid import CleanGrid
from viskores.filter.entity_extraction import ExtractGeometry
from viskores.testing import MakeTestDataSet


def test_uniform_by_box_inside():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    extract = ExtractGeometry()
    extract.SetImplicitFunction(Box((1.0, 1.0, 1.0), (3.0, 3.0, 3.0)))
    extract.SetExtractInside(True)
    extract.SetExtractBoundaryCells(False)
    extract.SetExtractOnlyBoundaryCells(False)

    output = extract.Execute(dataset)
    assert output.GetNumberOfCells() == 8

    clean = CleanGrid()
    clean.SetCompactPointFields(True)
    clean.SetMergePoints(False)
    clean_output = clean.Execute(output)

    cell_field = clean_output.GetField("cellvar")
    assert np.isclose(cell_field[0], 21.0)
    assert np.isclose(cell_field[7], 42.0)

    point_field = clean_output.GetField("pointvar")
    assert np.isclose(point_field[0], 99.0)
    assert np.isclose(point_field[7], 90.0)


def test_uniform_by_box_outside():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    extract = ExtractGeometry()
    extract.SetImplicitFunction(Box((1.0, 1.0, 1.0), (3.0, 3.0, 3.0)))
    extract.SetExtractInside(False)
    extract.SetExtractBoundaryCells(False)
    extract.SetExtractOnlyBoundaryCells(False)

    output = extract.Execute(dataset)
    assert output.GetNumberOfCells() == 56
    cell_field = output.GetField("cellvar")
    assert np.isclose(cell_field[0], 0.0)
    assert np.isclose(cell_field[55], 63.0)


def test_uniform_boundary_modes():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    box = Box((0.5, 0.5, 0.5), (3.5, 3.5, 3.5))

    extract = ExtractGeometry()
    extract.SetImplicitFunction(box)
    extract.SetExtractInside(True)
    extract.SetExtractBoundaryCells(True)
    extract.SetExtractOnlyBoundaryCells(False)
    include_boundary = extract.Execute(dataset)
    assert include_boundary.GetNumberOfCells() == 64
    cell_field = include_boundary.GetField("cellvar")
    assert np.isclose(cell_field[0], 0.0)
    assert np.isclose(cell_field[63], 63.0)

    extract = ExtractGeometry()
    extract.SetImplicitFunction(box)
    extract.SetExtractInside(True)
    extract.SetExtractBoundaryCells(True)
    extract.SetExtractOnlyBoundaryCells(True)
    boundary_only = extract.Execute(dataset)
    assert boundary_only.GetNumberOfCells() == 56
    cell_field = boundary_only.GetField("cellvar")
    assert np.isclose(cell_field[0], 0.0)
    assert np.isclose(cell_field[55], 63.0)


def main():
    test_uniform_by_box_inside()
    test_uniform_by_box_outside()
    test_uniform_boundary_modes()


if __name__ == "__main__":
    main()
