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

import viskores
from viskores.cont import DataSetBuilderExplicitIterative
from viskores.filter.geometry_refinement import Tube


def build_input():
    builder = DataSetBuilderExplicitIterative()

    ids = [builder.AddPoint((0, 0, 0)), builder.AddPoint((1, 0, 0)), builder.AddPoint((2, 0, 0))]
    builder.AddCell(viskores.CELL_SHAPE_POLY_LINE, ids)

    ids = [builder.AddPoint((0, 1, 0)), builder.AddPoint((1, 1, 0)), builder.AddPoint((2, 1, 0))]
    builder.AddCell(viskores.CELL_SHAPE_POLY_LINE, ids)

    ids = [builder.AddPoint((0, 0, 0))]
    builder.AddCell(viskores.CELL_SHAPE_POLY_LINE, ids)

    ids = [builder.AddPoint((0, 0, 0)), builder.AddPoint((0, 0, 0))]
    builder.AddCell(viskores.CELL_SHAPE_POLY_LINE, ids)

    dataset = builder.Create()
    dataset.AddPointField(
        "pointVar",
        np.array([0, 1, 2, 10, 11, 12, -1, -1, -1], dtype=np.float32),
    )
    dataset.AddCellField("cellVar", np.array([100, 110, -1, -1], dtype=np.float32))
    return dataset


def main():
    tube = Tube()
    tube.SetCapping(True)
    tube.SetNumberOfSides(3)
    tube.SetRadius(0.2)

    output = tube.Execute(build_input())

    assert output.GetNumberOfCoordinateSystems() == 1
    assert output.GetNumberOfPoints() == 22
    assert output.GetNumberOfCells() == 36

    np.testing.assert_allclose(
        output.GetField("pointVar"),
        np.array(
            [0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12],
            dtype=np.float32,
        ),
    )
    np.testing.assert_allclose(
        output.GetField("cellVar"),
        np.array([100] * 18 + [110] * 18, dtype=np.float32),
    )


if __name__ == "__main__":
    main()
