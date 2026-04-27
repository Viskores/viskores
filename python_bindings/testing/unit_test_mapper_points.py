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
from viskores.rendering import MapperPoint
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    mapper = MapperPoint()
    render_with_mapper(maker.Make3DUniformDataSet1(), "pointvar", mapper, "point_regular3d.png")

    variable_mapper = MapperPoint()
    variable_mapper.UseVariableRadius(True)
    variable_mapper.SetRadiusDelta(4.0)
    variable_mapper.SetRadius(0.25)
    render_with_mapper(
        maker.Make3DUniformDataSet1(),
        "pointvar",
        variable_mapper,
        "point_variable_regular3d.png",
    )

    cell_mapper = MapperPoint()
    cell_mapper.SetUseCells()
    cell_mapper.SetRadius(1.0)
    render_with_mapper(maker.Make3DExplicitDataSet7(), "cellvar", cell_mapper, "point_cells.png")


if __name__ == "__main__":
    main()

