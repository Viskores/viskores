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
import sys

import viskores.cont
from viskores.cont import ColorTable
from viskores.filter.contour import Contour
from viskores.rendering import (
    Actor,
    Camera,
    CanvasRayTracer,
    MapperRayTracer,
    MapperVolume,
    MapperWireframer,
    Scene,
    View3D,
)
from viskores.source import Tangle


def main():
    parser = argparse.ArgumentParser(description="Python port of examples/demo/Demo.cxx")
    parser.add_argument("--dims", type=int, nargs=3, default=(50, 50, 50))
    parser.add_argument("--iso-value", type=float, default=3.0)
    args, remaining_argv = parser.parse_known_args()

    # Let Viskores process its command line options, such as device selection.
    viskores.cont.Initialize(
        [sys.argv[0], *remaining_argv], viskores.cont.InitializeOptions.Strict
    )

    # Create a simple data set.
    tangle = Tangle()
    tangle.SetPointDimensions(args.dims)
    tangle_data = tangle.Execute()
    field_name = "tangle"

    # Set up a camera for rendering the input data.
    camera = Camera()
    camera.SetLookAt((0.5, 0.5, 0.5))
    camera.SetViewUp((0.0, 1.0, 0.0))
    camera.SetClippingRange(1.0, 10.0)
    camera.SetFieldOfView(60.0)
    camera.SetPosition((1.5, 1.5, 1.5))

    color_table = ColorTable("inferno")

    # Background color:
    background = (0.2, 0.2, 0.2, 1.0)

    # 2048x2048 pixels in the canvas:
    actor = Actor(tangle_data, field_name, color_table)
    scene = Scene()
    scene.AddActor(actor)
    canvas = CanvasRayTracer(2048, 2048)

    # Create a view and use it to render the input data.
    view = View3D(scene, MapperVolume(), canvas, camera, background)
    view.Paint()
    view.SaveAs("volume.png")

    # Compute an isosurface:
    contour = Contour()
    contour.SetIsoValue(args.iso_value)
    contour.SetActiveField(field_name)
    iso_data = contour.Execute(tangle_data)

    # Render a separate image with the output isosurface.
    iso_actor = Actor(iso_data, field_name, color_table)
    # By default, the actor will automatically scale the scalar range of the
    # color table to match that of the data. However, we are coloring by the
    # scalar that we just extracted a contour from, so we want the scalar range
    # to match that of the previous image.
    iso_actor.SetScalarRange(actor.GetScalarRange())
    iso_scene = Scene()
    iso_scene.AddActor(iso_actor)

    # Wireframe surface:
    iso_view = View3D(iso_scene, MapperWireframer(), canvas, camera, background)
    iso_view.Paint()
    iso_view.SaveAs("isosurface_wireframer.png")

    # Smooth surface:
    solid_view = View3D(iso_scene, MapperRayTracer(), canvas, camera, background)
    solid_view.Paint()
    solid_view.SaveAs("isosurface_raytracer.png")


if __name__ == "__main__":
    main()
