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
from viskores.rendering import GlyphType, MapperGlyphScalar
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    mapper = MapperGlyphScalar()
    mapper.SetGlyphType(GlyphType.Cube)
    render_with_mapper(maker.Make3DUniformDataSet1(), "pointvar", mapper, "glyph_scalar_regular3d.png")

    mapper.SetScaleByValue(True)
    mapper.SetScaleDelta(0.5)
    mapper.SetBaseSize(0.25)
    render_with_mapper(
        maker.Make3DUniformDataSet1(),
        "pointvar",
        mapper,
        "glyph_scalar_variable_regular3d.png",
    )

    mapper.SetGlyphType(GlyphType.Sphere)
    render_with_mapper(
        maker.Make3DUniformDataSet3((7, 7, 7)),
        "pointvar",
        mapper,
        "glyph_scalar_variable_spheres_regular3d.png",
    )

    mapper.SetGlyphType(GlyphType.Axes)
    render_with_mapper(
        maker.Make3DUniformDataSet3((7, 7, 7)),
        "pointvar",
        mapper,
        "glyph_scalar_variable_axes_regular3d.png",
    )

    mapper.SetGlyphType(GlyphType.Quad)
    mapper.SetBaseSize(5.0)
    mapper.SetScaleDelta(0.75)
    render_with_mapper(
        maker.Make3DUniformDataSet3((7, 7, 7)),
        "pointvar",
        mapper,
        "glyph_scalar_variable_quads_regular3d.png",
    )

    mapper.SetScaleDelta(0.5)
    mapper.SetScaleByValue(False)
    mapper.SetGlyphType(GlyphType.Cube)
    mapper.SetUseCells()
    mapper.SetBaseSize(1.0)
    render_with_mapper(maker.Make3DExplicitDataSet7(), "cellvar", mapper, "glyph_scalar_cells.png")


if __name__ == "__main__":
    main()
