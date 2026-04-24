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
from viskores.filter.contour import SliceMultiple


def make_structured_3d():
    dataset = viskores.create_uniform_dataset(
        (3, 3, 3), origin=(-1.0, -1.0, -1.0), spacing=(1.0, 1.0, 1.0)
    )
    points = dataset.GetCoordinateSystem().GetData().AsNumPy()
    point_scalars = np.array(
        [(p[2] * 9.0 + p[1] * 3.0 + p[0]) * 0.1 for p in points], dtype=np.float64
    )
    point_v3 = np.column_stack((points[:, 0] * 0.1, points[:, 1] * 0.1, points[:, 2] * 0.1))
    point_v4 = np.column_stack((point_v3, points[:, 0] * 0.1))
    dataset.AddPointField("pointScalars", point_scalars)
    dataset.AddPointField("pointV3", point_v3)
    dataset.AddPointField("pointV4", point_v4)
    return dataset


def main():
    dataset = make_structured_3d()
    slice_multiple = SliceMultiple()
    slice_multiple.AddImplicitFunction(viskores.Plane((0.0, 0.0, 0.0), (0.0, 0.0, 1.0)))
    slice_multiple.AddImplicitFunction(viskores.Plane((0.0, 0.0, 0.0), (0.0, 1.0, 0.0)))
    slice_multiple.AddImplicitFunction(viskores.Plane((0.0, 0.0, 0.0), (1.0, 0.0, 0.0)))
    result = slice_multiple.Execute(dataset)

    assert result.GetNumberOfPoints() == 27
    assert result.GetCoordinateSystem().GetData().AsNumPy().shape == (27, 3)
    assert result.GetNumberOfCells() == 24

    coords = result.GetCoordinateSystem().GetData().AsNumPy()
    expected_scalars = np.array(
        [(p[2] * 9.0 + p[1] * 3.0 + p[0]) * 0.1 for p in coords], dtype=np.float64
    )
    expected_v3 = np.column_stack((coords[:, 0] * 0.1, coords[:, 1] * 0.1, coords[:, 2] * 0.1))
    expected_v4 = np.column_stack((expected_v3, coords[:, 0] * 0.1))

    assert np.allclose(result.GetField("pointScalars"), expected_scalars, rtol=1e-5, atol=1e-5)
    assert np.allclose(result.GetField("pointV3"), expected_v3, rtol=1e-5, atol=1e-5)
    assert np.allclose(result.GetField("pointV4"), expected_v4, rtol=1e-5, atol=1e-5)


if __name__ == "__main__":
    main()
