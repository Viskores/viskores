#!/usr/bin/env python3

##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import argparse

import viskores


def main():
    parser = argparse.ArgumentParser(description="Python port of examples/demo/Demo.cxx")
    parser.add_argument("--dims", type=int, nargs=3, default=(50, 50, 50))
    parser.add_argument("--iso-value", type=float, default=3.0)
    args = parser.parse_args()

    tangle = viskores.Tangle()
    tangle.set_point_dimensions(args.dims)
    tangle_data = tangle.execute()
    field_name = "tangle"

    camera = viskores.Camera()
    camera.set_look_at((0.5, 0.5, 0.5))
    camera.set_view_up((0.0, 1.0, 0.0))
    camera.set_clipping_range(1.0, 10.0)
    camera.set_field_of_view(60.0)
    camera.set_position((1.5, 1.5, 1.5))

    color_table = viskores.ColorTable("inferno")
    background = (0.2, 0.2, 0.2, 1.0)

    actor = viskores.Actor(tangle_data, field_name, color_table)
    scene = viskores.Scene()
    scene.add_actor(actor)
    canvas = viskores.CanvasRayTracer(2048, 2048)

    view = viskores.View3D(scene, viskores.MapperVolume(), canvas, camera, background)
    view.paint()
    view.save_as("volume.png")

    contour = viskores.Contour()
    contour.set_iso_value(args.iso_value)
    contour.set_active_field(field_name)
    iso_data = contour.execute(tangle_data)

    iso_actor = viskores.Actor(iso_data, field_name, color_table)
    iso_actor.set_scalar_range(actor.get_scalar_range())
    iso_scene = viskores.Scene()
    iso_scene.add_actor(iso_actor)

    iso_view = viskores.View3D(
        iso_scene, viskores.MapperWireframer(), canvas, camera, background
    )
    iso_view.paint()
    iso_view.save_as("isosurface_wireframer.png")

    solid_view = viskores.View3D(
        iso_scene, viskores.MapperRayTracer(), canvas, camera, background
    )
    solid_view.paint()
    solid_view.save_as("isosurface_raytracer.png")


if __name__ == "__main__":
    main()
