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
from viskores.filter.contour import ClipWithImplicitFunction


def make_structured_2d():
    dataset = viskores.create_uniform_dataset((3, 3), coord_name="coordinates")
    scalars = np.ones(9, dtype=np.float32)
    scalars[4] = 0.0
    dataset.AddPointField("scalars", scalars)
    return dataset


def main():
    dataset = make_structured_2d()
    clip = ClipWithImplicitFunction()
    clip.SetImplicitFunction(viskores.Sphere((1.0, 1.0, 0.0), 0.5))
    clip.SetOffset(0.0)
    clip.SetFieldsToPass("scalars")
    output = clip.Execute(dataset)

    assert output.GetNumberOfCoordinateSystems() == 1
    assert output.GetNumberOfFields() == 2
    assert output.GetNumberOfCells() == 8

    values = output.GetField("scalars")
    expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25], dtype=np.float32)
    assert values.shape == (12,)
    assert np.allclose(values, expected, rtol=1e-5, atol=1e-5)

    clip = ClipWithImplicitFunction()
    clip.SetImplicitFunction(viskores.Sphere((1.0, 1.0, 0.0), 0.5))
    clip.SetInvertClip(True)
    clip.SetFieldsToPass("scalars")
    output = clip.Execute(dataset)
    expected = np.array([0, 0.25, 0.25, 0.25, 0.25], dtype=np.float32)
    assert output.GetNumberOfCells() == 4
    assert np.allclose(output.GetField("scalars"), expected, rtol=1e-5, atol=1e-5)


if __name__ == "__main__":
    main()
