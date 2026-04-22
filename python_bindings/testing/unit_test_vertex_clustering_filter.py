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

from viskores.filter.geometry_refinement import VertexClustering
from viskores.testing import MakeTestDataSet


def main():
    dataset = MakeTestDataSet().Make3DExplicitDataSetCowNose()

    clustering = VertexClustering()
    clustering.SetNumberOfDivisions((3, 3, 3))
    clustering.SetFieldsToPass(("pointvar", "cellvar"))
    output = clustering.Execute(dataset)

    assert output.GetNumberOfCoordinateSystems() == 1
    assert output.GetNumberOfPoints() == 7
    assert output.GetNumberOfCells() == 6

    np.testing.assert_allclose(
        output.GetCoordinateSystem().GetData().AsNumPy(),
        np.array(
            [
                [0.0174716, 0.0501928, 0.0930275],
                [0.0307091, 0.1521420, 0.0539249],
                [0.0174172, 0.1371240, 0.1245530],
                [0.0480879, 0.1518740, 0.1073340],
                [0.0180085, 0.2043600, 0.1453160],
                [-0.000129414, 0.00247137, 0.1765610],
                [0.0108188, 0.1527740, 0.1679140],
            ],
            dtype=np.float64,
        ),
        rtol=1e-5,
        atol=1e-5,
    )
    np.testing.assert_allclose(
        output.GetField("pointvar"),
        np.array([28.0, 19.0, 25.0, 15.0, 16.0, 21.0, 30.0], dtype=np.float32),
        rtol=1e-6,
        atol=1e-6,
    )
    np.testing.assert_allclose(
        output.GetField("cellvar"),
        np.array([145.0, 134.0, 138.0, 140.0, 149.0, 144.0], dtype=np.float32),
        rtol=1e-6,
        atol=1e-6,
    )


if __name__ == "__main__":
    main()
