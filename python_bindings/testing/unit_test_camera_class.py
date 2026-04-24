from viskores import Range
from viskores.rendering import Camera


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


if __name__ == "__main__":
    main()
