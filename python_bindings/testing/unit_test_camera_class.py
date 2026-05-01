##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

from viskores import Range
from viskores.rendering import Camera


def expect_raises(callable_object, *args):
    try:
        callable_object(*args)
    except Exception:
        return
    raise AssertionError("Expected an exception.")


def main():
    camera = Camera()
    camera.SetLookAt((0.0, 0.0, 0.0))
    camera.SetViewUp((0.0, 1.0, 0.0))
    camera.SetPosition((1.0, 2.0, 3.0))
    camera.SetClippingRange(1.0, 10.0)
    camera.SetFieldOfView(60.0)

    assert camera.GetLookAt() == (0.0, 0.0, 0.0)
    assert camera.GetViewUp() == (0.0, 1.0, 0.0)
    assert camera.GetPosition() == (1.0, 2.0, 3.0)
    assert camera.GetClippingRange() == Range(1.0, 10.0)
    assert camera.GetFieldOfView() == 60.0

    assert camera.GetMode() == Camera.Mode.ThreeD

    camera.SetModeTo2D()
    assert camera.GetMode() == Camera.Mode.TwoD

    camera.SetViewRange2D(-0.5, 4.5, -0.5, 4.5)
    assert camera.GetMode() == Camera.Mode.TwoD
    assert camera.GetViewRange2D() == (-0.5, 4.5, -0.5, 4.5)

    camera.SetXScale(2.0)
    assert camera.GetXScale() == 2.0
    assert camera.GetMode() == Camera.Mode.TwoD

    camera.SetMode(Camera.Mode.ThreeD)
    assert camera.GetMode() == Camera.Mode.ThreeD

    expect_raises(camera.SetLookAt, (1.0, 2.0))
    expect_raises(camera.SetLookAt, (1.0, 2.0, 3.0, 4.0))
    expect_raises(camera.ResetToBounds, (0.0, 1.0, 0.0, 1.0, 0.0))
    expect_raises(camera.ResetToBounds, (0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 2.0))


if __name__ == "__main__":
    main()
