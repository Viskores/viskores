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
from viskores.filter.field_transform import Warp


def coordinate_components(dims, spacing):
    x_values = []
    y_values = []
    z_values = []
    for z_index in range(dims[2]):
        for y_index in range(dims[1]):
            for x_index in range(dims[0]):
                x_values.append(x_index * spacing[0])
                y_values.append(y_index * spacing[1])
                z_values.append(z_index * spacing[2])
    return (
        np.asarray(x_values, dtype=np.float64),
        np.asarray(y_values, dtype=np.float64),
        np.asarray(z_values, dtype=np.float64),
    )


def main():
    dims = (3, 3, 3)
    spacing = (0.5, 0.5, 0.5)
    ds = viskores.create_uniform_dataset(dims, spacing=spacing)

    x_coords, y_coords, z_coords = coordinate_components(dims, spacing)
    scalar_factor = np.arange(ds.GetNumberOfPoints(), dtype=np.float64)
    vec1 = np.stack([x_coords, y_coords, y_coords], axis=1)
    vec2 = np.stack(
        [np.zeros_like(scalar_factor), np.zeros_like(scalar_factor), scalar_factor], axis=1
    )
    normal = np.tile(np.asarray([[0.0, 0.0, 1.0]], dtype=np.float64), (ds.GetNumberOfPoints(), 1))

    ds.AddPointField("vec1", vec1)
    ds.AddPointField("vec2", vec2)
    ds.AddPointField("scalarfactor", scalar_factor)
    ds.AddPointField("normal", normal)

    warp_scalar = Warp()
    warp_scalar.SetUseCoordinateSystemAsField(True)
    warp_scalar.SetChangeCoordinateSystem(False)
    warp_scalar.SetOutputFieldName("warped_coords")
    warp_scalar.SetConstantDirection((0.0, 0.0, 1.0))
    warp_scalar.SetScaleField("scalarfactor")
    warp_scalar.SetScaleFactor(2.0)
    scalar_out = warp_scalar.Execute(ds).GetField("warped_coords")

    expected_scalar = np.stack([x_coords, y_coords, z_coords + 2.0 * scalar_factor], axis=1)
    np.testing.assert_allclose(scalar_out, expected_scalar)

    warp_vector = Warp()
    warp_vector.SetActiveField("vec1")
    warp_vector.SetUseCoordinateSystemAsField(False)
    warp_vector.SetChangeCoordinateSystem(False)
    warp_vector.SetOutputFieldName("warped_vectors")
    warp_vector.SetDirectionField("vec2")
    warp_vector.SetScaleFactor(2.0)
    vector_out = warp_vector.Execute(ds).GetField("warped_vectors")

    expected_vector = np.stack([x_coords, y_coords, y_coords + 2.0 * scalar_factor], axis=1)
    np.testing.assert_allclose(vector_out, expected_vector)


if __name__ == "__main__":
    main()
