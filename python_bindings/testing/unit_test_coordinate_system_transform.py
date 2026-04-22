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
from viskores.filter.field_transform import (
    CylindricalCoordinateTransform,
    SphericalCoordinateTransform,
)


def make_cart_dataset():
    dims = (5, 5, 5)
    spacing = (0.25, 0.25, 0.25)
    ds = viskores.create_uniform_dataset(dims, spacing=spacing)
    return ds, ds.GetCoordinateSystem().GetData().AsNumPy()


def make_cyl_dataset():
    points = []
    dim = 5
    radius = 1.0
    for z_index in range(dim):
        z_value = z_index / (dim - 1)
        for i in range(dim):
            theta = (2.0 * np.pi) * (i / (dim - 1))
            points.append([radius, theta, z_value])
    ds = viskores.create_uniform_dataset((5, 5, 1))
    ds.AddPointField("coordinates", np.asarray(points, dtype=np.float32))
    return ds, np.asarray(points, dtype=np.float32)


def make_sph_dataset():
    eps = np.finfo(np.float32).eps
    thetas = np.asarray([eps, np.pi / 4.0, np.pi / 3.0, np.pi / 2.0, np.pi - eps], dtype=np.float32)
    phis = np.asarray(
        [eps, 2.0 * np.pi / 4.0, 2.0 * np.pi / 3.0, 2.0 * np.pi / 2.0, 2.0 * np.pi - eps],
        dtype=np.float32,
    )
    points = []
    for theta in thetas:
        for phi in phis:
            points.append([1.0, theta, phi])
    ds = viskores.create_uniform_dataset((5, 5, 1))
    ds.AddPointField("coordinates", np.asarray(points, dtype=np.float32))
    return ds, np.asarray(points, dtype=np.float32)


def assert_angle_equivalent(actual, expected, angle_columns):
    assert actual.shape == expected.shape
    for column in range(actual.shape[1]):
        if column in angle_columns:
            diff = np.mod(actual[:, column] - expected[:, column], 2.0 * np.pi)
            wrapped = np.minimum(diff, 2.0 * np.pi - diff)
            np.testing.assert_allclose(wrapped, 0.0, atol=1e-5)
        else:
            np.testing.assert_allclose(actual[:, column], expected[:, column], atol=1e-5)


def main():
    cart_ds, cart_points = make_cart_dataset()
    cyl = CylindricalCoordinateTransform()
    cyl.SetUseCoordinateSystemAsField(True)
    cyl.SetCartesianToCylindrical()
    cart_to_cyl = cyl.Execute(cart_ds)
    cyl.SetCylindricalToCartesian()
    cyl_to_cart = cyl.Execute(cart_to_cyl)
    np.testing.assert_allclose(cyl_to_cart.GetCoordinateSystem().GetData().AsNumPy(), cart_points, atol=1e-5)

    cyl_ds, cyl_points = make_cyl_dataset()
    cyl.SetUseCoordinateSystemAsField(False)
    cyl.SetActiveField("coordinates")
    cyl.SetCylindricalToCartesian()
    cyl_to_cart = cyl.Execute(cyl_ds)
    cyl.SetUseCoordinateSystemAsField(True)
    cyl.SetCartesianToCylindrical()
    cart_to_cyl = cyl.Execute(cyl_to_cart)
    assert_angle_equivalent(cart_to_cyl.GetCoordinateSystem().GetData().AsNumPy(), cyl_points, {1})

    sph = SphericalCoordinateTransform()
    sph.SetUseCoordinateSystemAsField(True)
    sph.SetCartesianToSpherical()
    cart_to_sph = sph.Execute(cart_ds)
    sph.SetSphericalToCartesian()
    sph_to_cart = sph.Execute(cart_to_sph)
    np.testing.assert_allclose(sph_to_cart.GetCoordinateSystem().GetData().AsNumPy(), cart_points, atol=1e-5)

    sph_ds, sph_points = make_sph_dataset()
    sph.SetUseCoordinateSystemAsField(False)
    sph.SetActiveField("coordinates")
    sph.SetSphericalToCartesian()
    sph_to_cart = sph.Execute(sph_ds)
    sph.SetUseCoordinateSystemAsField(True)
    sph.SetCartesianToSpherical()
    cart_to_sph = sph.Execute(sph_to_cart)
    assert_angle_equivalent(cart_to_sph.GetCoordinateSystem().GetData().AsNumPy(), sph_points, {1, 2})


if __name__ == "__main__":
    main()
