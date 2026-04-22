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

from viskores.cont import Association
from viskores.filter.vector_analysis import SurfaceNormals
from viskores.testing import MakeTestDataSet


def main():
    dataset = MakeTestDataSet().Make3DExplicitDataSetPolygonal()

    surface_normals = SurfaceNormals()
    result = surface_normals.Execute(dataset)
    assert result.HasField("Normals")

    surface_normals.SetGenerateCellNormals(True)
    surface_normals.SetGeneratePointNormals(False)
    result = surface_normals.Execute(dataset)
    assert result.HasField("Normals")

    surface_normals.SetGeneratePointNormals(True)
    surface_normals.SetAutoOrientNormals(True)
    result = surface_normals.Execute(dataset)
    assert result.HasField("Normals")

    expected_point = np.array(
        [
            [-0.8165, -0.4082, -0.4082],
            [-0.2357, -0.9714, 0.0286],
            [0.0, -0.1691, 0.9856],
            [-0.8660, 0.0846, 0.4928],
            [0.0, -0.1691, -0.9856],
            [0.0, 0.9856, -0.1691],
            [0.8165, 0.4082, 0.4082],
            [0.8165, -0.4082, -0.4082],
        ],
        dtype=np.float32,
    )
    expected_cell = np.array(
        [
            [-0.707, -0.500, 0.500],
            [-0.707, -0.500, 0.500],
            [0.707, 0.500, -0.500],
            [0.0, -0.707, -0.707],
            [0.0, -0.707, -0.707],
            [0.0, 0.707, 0.707],
            [-0.707, 0.500, -0.500],
            [0.707, -0.500, 0.500],
        ],
        dtype=np.float32,
    )

    point_normals = result.GetField("Normals")
    cell_normals = result.GetField("Normals", association=Association.CELLS)
    np.testing.assert_allclose(point_normals, expected_point, rtol=1e-3, atol=1e-3)
    np.testing.assert_allclose(cell_normals, expected_cell, rtol=1e-3, atol=1e-3)


if __name__ == "__main__":
    main()
