##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

from viskores import CELL_SHAPE_HEXAHEDRON, CELL_SHAPE_TRIANGLE
from viskores.python_convenience import create_uniform_dataset
from viskores.cont import DataSetBuilderExplicit, UncertainCellSet, UnknownCellSet
from viskores.testing import can_narrow_to_structured_cell_set


def main():
    structured = create_uniform_dataset((3, 5, 7))
    structured_cell_set = structured.GetCellSet()
    assert isinstance(structured_cell_set, UnknownCellSet)
    assert structured_cell_set.IsValid()
    assert structured_cell_set.GetNumberOfPoints() == 105
    assert structured_cell_set.GetNumberOfCells() == 48
    assert structured_cell_set.GetCellShape(0) == CELL_SHAPE_HEXAHEDRON
    assert structured_cell_set.GetNumberOfPointsInCell(0) == 8
    assert len(structured_cell_set.GetCellPointIds(0)) == 8
    uncertain_structured_cell_set = UncertainCellSet(structured_cell_set)
    assert isinstance(uncertain_structured_cell_set, UncertainCellSet)
    assert isinstance(uncertain_structured_cell_set, UnknownCellSet)
    assert uncertain_structured_cell_set.GetNumberOfCells() == structured_cell_set.GetNumberOfCells()
    assert can_narrow_to_structured_cell_set(uncertain_structured_cell_set)

    explicit = DataSetBuilderExplicit.Create(
        [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0)],
        [CELL_SHAPE_TRIANGLE],
        [3],
        [0, 1, 2],
    )
    explicit_cell_set = explicit.GetCellSet()
    assert explicit_cell_set.IsValid()
    assert explicit_cell_set.GetNumberOfPoints() == 3
    assert explicit_cell_set.GetNumberOfCells() == 1
    assert explicit_cell_set.GetCellShape(0) == CELL_SHAPE_TRIANGLE
    assert explicit_cell_set.GetCellPointIds(0) == [0, 1, 2]
    assert not can_narrow_to_structured_cell_set(explicit_cell_set)


if __name__ == "__main__":
    main()
