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
from viskores.io import ImageReaderPNG, ImageReaderPNM, ImageWriterPNG, ImageWriterPNM, PixelDepth
from viskores.rendering import Canvas


def check_filled_image(dataset, field_name, canvas):
    assert dataset.HasField(field_name, Association.POINTS)
    point_field = dataset.GetFieldObject(field_name, Association.POINTS)
    assert point_field.GetNumberOfValues() == canvas.GetWidth() * canvas.GetHeight()
    pixels = dataset.GetField(field_name)
    colors = canvas.GetColorBuffer()
    assert pixels.shape == colors.shape
    assert np.allclose(pixels, colors)


def check_png_roundtrip(canvas, filename, pixel_depth):
    writer = ImageWriterPNG(str(filename))
    writer.SetPixelDepth(pixel_depth)
    writer.WriteDataSet(canvas.GetDataSet())

    reader = ImageReaderPNG(str(filename))
    dataset = reader.ReadDataSet()
    check_filled_image(dataset, reader.GetPointFieldName(), canvas)


def check_pnm_roundtrip(canvas, filename, pixel_depth):
    writer = ImageWriterPNM(str(filename))
    writer.SetPixelDepth(pixel_depth)
    writer.WriteDataSet(canvas.GetDataSet())

    reader = ImageReaderPNM(str(filename))
    dataset = reader.ReadDataSet()
    check_filled_image(dataset, reader.GetPointFieldName(), canvas)


def main():
    viskores.cont.Initialize(["unit_test_image_writer.py"])

    canvas = Canvas(16, 16)
    canvas.SetBackgroundColor((1.0, 0.0, 0.0, 1.0))
    canvas.Clear()
    canvas.AddLine(-0.9, 0.9, 0.9, -0.9, 2.0, (0.0, 0.0, 0.0, 1.0))
    canvas.AddColorBar((-0.8, -0.6, -0.8, 0.8, 0.0, 0.0), ColorTable("inferno"), False)
    canvas.BlendBackground()

    dataset = canvas.GetDataSet("pixel-color")
    check_filled_image(dataset, "pixel-color", canvas)

    with tempfile.TemporaryDirectory() as temp_dir:
        temp_dir = Path(temp_dir)
        check_png_roundtrip(canvas, temp_dir / "pngRGB8Test.png", PixelDepth.PIXEL_8)
        check_png_roundtrip(canvas, temp_dir / "pngRGB16Test.png", PixelDepth.PIXEL_16)
        check_pnm_roundtrip(canvas, temp_dir / "pnmRGB8Test.pnm", PixelDepth.PIXEL_8)
        check_pnm_roundtrip(canvas, temp_dir / "pnmRGB16Test.pnm", PixelDepth.PIXEL_16)


if __name__ == "__main__":
    main()
