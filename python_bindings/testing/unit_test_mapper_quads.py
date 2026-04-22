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
from viskores.rendering import MapperQuad
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    render_with_mapper(
        maker.Make3DRegularDataSet0(), "pointvar", MapperQuad(), "quad_regular3d.png"
    )
    render_with_mapper(
        maker.Make3DRectilinearDataSet0(), "pointvar", MapperQuad(), "quad_rectilinear3d.png"
    )
    render_with_mapper(
        maker.Make3DExplicitDataSet4(), "pointvar", MapperQuad(), "quad_explicit3d.png"
    )
    render_with_mapper(
        maker.Make3DExplicitDataSet5(), "cellvar", MapperQuad(), "quad_mixed3d.png"
    )
    render_with_mapper(
        maker.Make2DUniformDataSet1(), "pointvar", MapperQuad(), "quad_uniform2d.png"
    )


if __name__ == "__main__":
    main()

