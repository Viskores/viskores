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
from viskores.rendering import MapperRayTracer
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    render_with_mapper(
        maker.Make3DRegularDataSet0(),
        "pointvar",
        MapperRayTracer(),
        "raytracer_regular3d.png",
    )
    render_with_mapper(
        maker.Make3DRectilinearDataSet0(),
        "pointvar",
        MapperRayTracer(),
        "raytracer_rectilinear3d.png",
    )
    render_with_mapper(
        maker.Make3DExplicitDataSet4(),
        "pointvar",
        MapperRayTracer(),
        "raytracer_explicit3d.png",
    )
    render_with_mapper(
        maker.Make3DExplicitDataSet7(),
        "cellvar",
        MapperRayTracer(),
        "raytracer_vertex_cells.png",
    )
    render_with_mapper(
        maker.Make2DUniformDataSet1(),
        "pointvar",
        MapperRayTracer(),
        "raytracer_uniform2d.png",
    )


if __name__ == "__main__":
    main()
