##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import tempfile
from pathlib import Path

import numpy as np

import viskores.cont
from viskores import Association
from viskores.cont import ColorTable
from viskores.io import ImageReaderHDF5, ImageWriterHDF5, PixelDepth
from viskores.rendering import Canvas


def check_filled_image(dataset, field_name, canvas):
    assert dataset.HasField(field_name, Association.POINTS)
    point_field = dataset.GetFieldObject(field_name, Association.POINTS)
    assert point_field.GetNumberOfValues() == canvas.GetWidth() * canvas.GetHeight()
    pixels = dataset.GetField(field_name)
    colors = canvas.GetColorBuffer()
    assert pixels.shape == colors.shape
    assert np.allclose(pixels, colors)


def check_hdf5_roundtrip(canvas, filename, pixel_depth):
    threw = False
    try:
        ImageWriterHDF5(str(filename)).WriteDataSet(viskores.cont.DataSet(), "color")
    except RuntimeError:
        threw = True
    assert threw

    writer = ImageWriterHDF5(str(filename))
    writer.SetPixelDepth(pixel_depth)
    writer.WriteDataSet(canvas.GetDataSet(), "color")

    reader = ImageReaderHDF5(str(filename))
    dataset = reader.ReadDataSet()
    check_filled_image(dataset, reader.GetPointFieldName(), canvas)


def main():
    viskores.cont.Initialize(["unit_test_hdf5_image.py"])

    canvas = Canvas(16, 16)
    canvas.SetBackgroundColor((1.0, 0.0, 0.0, 1.0))
    canvas.Clear()
    canvas.AddLine(-0.9, 0.9, 0.9, -0.9, 2.0, (0.0, 0.0, 0.0, 1.0))
    canvas.AddColorBar((-0.8, -0.6, -0.8, 0.8, 0.0, 0.0), ColorTable("inferno"), False)
    canvas.BlendBackground()

    with tempfile.TemporaryDirectory() as temp_dir:
        temp_dir = Path(temp_dir)
        check_hdf5_roundtrip(canvas, temp_dir / "hdf5RGB8Test.h5", PixelDepth.PIXEL_8)
        check_hdf5_roundtrip(canvas, temp_dir / "hdf5RGB16Test.h5", PixelDepth.PIXEL_16)


if __name__ == "__main__":
    main()
