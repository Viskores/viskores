##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.rendering import MapperCylinder
from viskores.testing import MakeTestDataSet

from render_test_utils import render_with_mapper


def main():
    maker = MakeTestDataSet()
    mapper = MapperCylinder()
    render_with_mapper(maker.Make3DRegularDataSet0(), "pointvar", mapper, "cylinder_regular3d.png")
    render_with_mapper(
        maker.Make3DRectilinearDataSet0(),
        "pointvar",
        mapper,
        "cylinder_rectilinear3d.png",
    )
    render_with_mapper(maker.Make3DExplicitDataSet4(), "pointvar", mapper, "cylinder_explicit_hex.png")

    render_with_mapper(
        maker.Make3DExplicitDataSet8(),
        "cellvar",
        mapper,
        "cylinder_explicit_lines.png",
    )
    render_with_mapper(
        maker.Make3DExplicitDataSet5(),
        "cellvar",
        mapper,
        "cylinder_explicit_zoo.png",
    )

    mapper.SetRadius(0.1)
    render_with_mapper(
        maker.Make3DExplicitDataSet8(),
        "cellvar",
        mapper,
        "cylinder_static_radius.png",
    )

    mapper.UseVariableRadius(True)
    mapper.SetRadius(2.0)
    render_with_mapper(
        maker.Make3DExplicitDataSet8(),
        "cellvar",
        mapper,
        "cylinder_variable_radius.png",
    )


if __name__ == "__main__":
    main()
