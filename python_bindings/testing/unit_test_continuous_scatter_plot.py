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
from viskores import CELL_SHAPE_TETRA
from viskores.cont import DataSetBuilderExplicit
from viskores.filter.density_estimate import ContinuousScatterPlot


def main():
    viskores.cont.Initialize(["unit_test_continuous_scatter_plot.py"])

    dataset = DataSetBuilderExplicit.Create(
        [
            (0.0, 0.0, 0.0),
            (2.0, 0.0, 0.0),
            (2.0, 2.0, 0.0),
            (1.0, 0.0, 2.0),
        ],
        [CELL_SHAPE_TETRA],
        [4],
        [0, 1, 2, 3],
    )
    dataset.AddPointField("scalar1", np.array([0.0, 1.0, 0.0, -2.0], dtype=np.float32))
    dataset.AddPointField("scalar2", np.array([-1.0, 0.0, 2.0, 0.0], dtype=np.float32))

    scatter_plot = ContinuousScatterPlot()
    scatter_plot.SetActiveFieldsPair("scalar1", "scalar2")
    output = scatter_plot.Execute(dataset)

    assert output.GetNumberOfCells() == 4
    assert output.GetNumberOfPoints() == 5

    positions = output.GetCoordinateSystem().GetData().AsNumPy()
    np.testing.assert_allclose(
        positions,
        np.array(
            [
                [0.0, -1.0, 0.0],
                [1.0, 0.0, 0.0],
                [0.0, 2.0, 0.0],
                [-2.0, 0.0, 0.0],
                [0.0, 0.0, 0.0],
            ],
            dtype=np.float32,
        ),
    )

    cell_set = output.GetCellSet()
    assert cell_set.GetCellPointIds(0) == [0, 1, 4]
    assert cell_set.GetCellPointIds(1) == [1, 2, 4]
    assert cell_set.GetCellPointIds(2) == [2, 3, 4]
    assert cell_set.GetCellPointIds(3) == [3, 0, 4]

    np.testing.assert_allclose(
        output.GetField("density"),
        np.array([0.0, 0.0, 0.0, 0.0, 0.888889], dtype=np.float64),
        rtol=1e-5,
        atol=1e-5,
    )


if __name__ == "__main__":
    main()

