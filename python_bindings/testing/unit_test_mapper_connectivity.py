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
from viskores.rendering import MapperConnectivity
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    render_with_mapper(
        maker.Make3DRegularDataSet0(),
        "pointvar",
        MapperConnectivity(),
        "connectivity_regular3d.png",
    )
    render_with_mapper(
        maker.Make3DRectilinearDataSet0(),
        "pointvar",
        MapperConnectivity(),
        "connectivity_rectilinear3d.png",
    )
    render_with_mapper(
        maker.Make3DExplicitDataSetZoo(),
        "pointvar",
        MapperConnectivity(),
        "connectivity_explicit3d.png",
    )


if __name__ == "__main__":
    main()

