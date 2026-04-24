import numpy as np

from viskores.cont import CoordinateSystem, DataSet


def main():
    points = np.array(
        [[0.0, 0.0, 0.0], [1.0, 2.0, 3.0], [4.0, 5.0, 6.0]],
        dtype=np.float32,
    )
    coords = CoordinateSystem("coords", points)
    assert coords.GetName() == "coords"
    assert coords.IsPointField()
    assert coords.GetNumberOfPoints() == 3
    np.testing.assert_allclose(coords.GetData().AsNumPy(), points)
    assert coords.GetBounds() == (0.0, 4.0, 0.0, 5.0, 0.0, 6.0)

    ranges = coords.GetRange()
    assert ranges == [(0.0, 4.0), (0.0, 5.0), (0.0, 6.0)]

    uniform = CoordinateSystem("uniform", (2, 3, 4), (1.0, 2.0, 3.0), (0.5, 1.0, 1.5))
    assert uniform.GetNumberOfPoints() == 24
    assert uniform.GetBounds() == (1.0, 1.5, 2.0, 4.0, 3.0, 7.5)

    dataset = DataSet()
    dataset.AddCoordinateSystem(coords)
    returned = dataset.GetCoordinateSystem()
    assert isinstance(returned, CoordinateSystem)
    assert returned.GetName() == "coords"
    np.testing.assert_allclose(returned.GetData().AsNumPy(), points)


if __name__ == "__main__":
    main()
