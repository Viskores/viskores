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
from viskores.rendering import GlyphType, MapperGlyphVector
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    mapper = MapperGlyphVector()
    mapper.SetGlyphType(GlyphType.Arrow)
    mapper.SetScaleByValue(True)
    mapper.SetScaleDelta(0.5)
    mapper.SetBaseSize(0.02)
    render_with_mapper(
        maker.Make3DExplicitDataSetCowNose(),
        "point_vectors",
        mapper,
        "glyph_vector_points_arrows_cownose.png",
    )


if __name__ == "__main__":
    main()
