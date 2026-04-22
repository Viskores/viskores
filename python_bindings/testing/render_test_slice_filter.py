##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import viskores
from render_test_utils import render_with_mapper
from viskores.filter.contour import Slice
from viskores.filter.geometry_refinement import Tetrahedralize
from viskores.rendering import MapperRayTracer
from viskores.source import Wavelet


def render_slice(result, output_name):
    render_with_mapper(result, "RTData", MapperRayTracer(), output_name)


def main():
    wavelet = Wavelet()
    wavelet.SetExtent((-8, -8, -8), (8, 8, 8))
    dataset = wavelet.Execute()

    slice_filter = Slice()
    slice_filter.SetImplicitFunction(viskores.Plane((1.0, 1.0, 1.0)))
    render_slice(slice_filter.Execute(dataset), "slice_structured_points_plane.png")

    slice_filter = Slice()
    slice_filter.SetImplicitFunction(viskores.Sphere(8.5))
    render_slice(slice_filter.Execute(dataset), "slice_structured_points_sphere.png")

    tetrahedralize = Tetrahedralize()
    unstructured = tetrahedralize.Execute(dataset)

    slice_filter = Slice()
    slice_filter.SetImplicitFunction(viskores.Plane((1.0, 1.0, 1.0)))
    render_slice(slice_filter.Execute(unstructured), "slice_unstructured_grid_plane.png")

    slice_filter = Slice()
    slice_filter.SetImplicitFunction(viskores.Cylinder((0.0, 1.0, 0.0), 8.5))
    render_slice(slice_filter.Execute(unstructured), "slice_unstructured_grid_cylinder.png")


if __name__ == "__main__":
    main()
