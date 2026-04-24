##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from render_test_utils import render_with_mapper
from viskores.cont import ArrayCopy, ArrayHandleSOAVec3f
from viskores.rendering import GlyphType, MapperGlyphScalar, MapperRayTracer
from viskores.source import Wavelet


def render_structured_grid(dataset, suffix):
    render_with_mapper(dataset, "RTData", MapperRayTracer(), f"alt_coords_{suffix}_raytracer.png")

    glyph_mapper = MapperGlyphScalar()
    glyph_mapper.SetGlyphType(GlyphType.Sphere)
    glyph_mapper.SetScaleByValue(True)
    glyph_mapper.SetBaseSize(0.5)
    render_with_mapper(dataset, "RTData", glyph_mapper, f"alt_coords_{suffix}_glyph.png")


def main():
    wavelet = Wavelet()
    wavelet.SetExtent((0, 0, 0), (10, 10, 10))
    dataset = wavelet.Execute()

    render_structured_grid(dataset, "default")

    coords = dataset.GetCoordinateSystem()
    soa_coords = ArrayHandleSOAVec3f()
    ArrayCopy(coords.GetData(), soa_coords)
    coords.SetData(soa_coords)
    dataset.AddCoordinateSystem(coords)

    assert dataset.GetCoordinateSystem().GetData().IsStorageTypeSOA()
    render_structured_grid(dataset, "soa")

    coords = dataset.GetCoordinateSystem()
    strided_coords = coords.GetData().ExtractArrayFromComponents()
    coords.SetData(strided_coords)
    dataset.AddCoordinateSystem(coords)
    render_structured_grid(dataset, "strided")


if __name__ == "__main__":
    main()
