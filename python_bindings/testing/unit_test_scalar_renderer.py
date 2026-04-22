##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.rendering import Camera, ScalarRenderer
from viskores.testing import MakeTestDataSet


def dataset_bounds(dataset):
    coords = dataset.GetCoordinateSystem().GetData().AsNumPy()
    return (
        float(coords[:, 0].min()),
        float(coords[:, 0].max()),
        float(coords[:, 1].min()),
        float(coords[:, 1].max()),
        float(coords[:, 2].min()),
        float(coords[:, 2].max()),
    )


def main():
    maker = MakeTestDataSet()
    dataset = maker.Make3DRegularDataSet0()

    camera = Camera()
    camera.ResetToBounds(dataset_bounds(dataset))
    camera.Azimuth(-40.0)
    camera.Elevation(15.0)

    renderer = ScalarRenderer()
    renderer.SetInput(dataset)
    result = renderer.Render(camera)

    assert result.Width > 0
    assert result.Height > 0
    assert result.Depths.shape == (result.Width * result.Height,)
    assert len(result.ScalarNames) > 0
    assert "pointvar" in list(result.ScalarNames)

    output = result.ToDataSet()
    assert output.GetNumberOfCells() == result.Width * result.Height
    assert output.GetNumberOfPoints() == (result.Width + 1) * (result.Height + 1)
    assert output.GetNumberOfFields() >= 2


if __name__ == "__main__":
    main()
