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
from viskores.filter.field_transform import PointTransform


def coordinates(dims, spacing):
    coords = []
    for z_index in range(dims[2]):
        for y_index in range(dims[1]):
            for x_index in range(dims[0]):
                coords.append(
                    [
                        x_index * spacing[0],
                        y_index * spacing[1],
                        z_index * spacing[2],
                    ]
                )
    return np.asarray(coords, dtype=np.float64)


def main():
    dims = (3, 3, 3)
    spacing = (0.5, 0.5, 0.5)
    ds = viskores.create_uniform_dataset(dims, spacing=spacing)
    base = coordinates(dims, spacing)

    translate = PointTransform()
    translate.SetUseCoordinateSystemAsField(True)
    translate.SetChangeCoordinateSystem(False)
    translate.SetOutputFieldName("translation")
    translate.SetTranslation((1.0, -2.0, 0.5))
    translated = translate.Execute(ds).GetField("translation")
    np.testing.assert_allclose(translated, base + np.asarray([1.0, -2.0, 0.5]))

    scale = PointTransform()
    scale.SetUseCoordinateSystemAsField(True)
    scale.SetChangeCoordinateSystem(False)
    scale.SetOutputFieldName("scale")
    scale.SetScale((2.0, 3.0, 4.0))
    scaled = scale.Execute(ds).GetField("scale")
    np.testing.assert_allclose(scaled, base * np.asarray([2.0, 3.0, 4.0]))

    rotate = PointTransform()
    rotate.SetUseCoordinateSystemAsField(True)
    rotate.SetChangeCoordinateSystem(False)
    rotate.SetOutputFieldName("rotation")
    rotate.SetRotation(90.0, (0.0, 0.0, 1.0))
    rotated = rotate.Execute(ds).GetField("rotation")
    expected_rotated = np.stack([-base[:, 1], base[:, 0], base[:, 2]], axis=1)
    np.testing.assert_allclose(rotated, expected_rotated, atol=1e-6)


if __name__ == "__main__":
    main()
