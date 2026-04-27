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

from viskores.filter.geometry_refinement import Shrink
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    shrink = Shrink()
    shrink.SetFieldsToPass(("pointvar", "cellvar"))
    assert np.isclose(shrink.GetShrinkFactor(), 0.5)

    shrink.SetShrinkFactor(1.5)
    assert np.isclose(shrink.GetShrinkFactor(), 1.0)

    shrink.SetShrinkFactor(-0.5)
    assert np.isclose(shrink.GetShrinkFactor(), 0.0)

    shrink.SetShrinkFactor(0.5)
    output = shrink.Execute(maker.Make3DExplicitDataSet0())

    assert output.GetNumberOfCells() == 2
    assert output.GetNumberOfPoints() == 7
    assert np.allclose(output.GetField("cellvar"), np.array([100.1, 100.2], dtype=np.float32))
    np.testing.assert_allclose(
        output.GetField("pointvar"),
        np.array([10.1, 20.1, 30.2, 30.2, 20.1, 40.2, 50.3], dtype=np.float32),
        rtol=1e-6,
        atol=1e-6,
    )
    np.testing.assert_allclose(
        output.GetCoordinateSystem().GetData().AsNumPy(),
        np.array(
            [
                [0.333333, 0.166666, 0.0],
                [0.833333, 0.166666, 0.0],
                [0.833333, 0.666666, 0.0],
                [1.25, 1.0, 0.0],
                [1.25, 0.5, 0.0],
                [1.75, 1.0, 0.0],
                [1.75, 1.5, 0.0],
            ],
            dtype=np.float32,
        ),
        rtol=1e-5,
        atol=1e-5,
    )

    shrink = Shrink()
    shrink.SetFieldsToPass(("pointvar", "cellvar"))
    shrink.SetShrinkFactor(0.2)
    output = shrink.Execute(maker.Make3DUniformDataSet0())

    assert output.GetNumberOfCells() == 4
    assert output.GetNumberOfPoints() == 32
    np.testing.assert_allclose(
        output.GetField("cellvar"),
        np.array([100.1, 100.2, 100.3, 100.4], dtype=np.float32),
        rtol=1e-6,
        atol=1e-6,
    )
    np.testing.assert_allclose(
        output.GetField("pointvar")[:8],
        np.array([10.1, 20.1, 50.2, 40.1, 70.2, 80.2, 110.3, 100.3], dtype=np.float32),
        rtol=1e-6,
        atol=1e-6,
    )
    np.testing.assert_allclose(
        output.GetCoordinateSystem().GetData().AsNumPy()[:8],
        np.array(
            [
                [0.4, 0.4, 0.4],
                [0.6, 0.4, 0.4],
                [0.6, 0.6, 0.4],
                [0.4, 0.6, 0.4],
                [0.4, 0.4, 0.6],
                [0.6, 0.4, 0.6],
                [0.6, 0.6, 0.6],
                [0.4, 0.6, 0.6],
            ],
            dtype=np.float32,
        ),
        rtol=1e-6,
        atol=1e-6,
    )


if __name__ == "__main__":
    main()
