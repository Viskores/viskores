##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import tempfile
from pathlib import Path

from viskores.cont import ColorTable
from viskores.rendering import Actor, Camera, CanvasRayTracer, Scene, View3D


def make_camera():
    camera = Camera()
    camera.SetLookAt((0.5, 0.5, 0.5))
    camera.SetViewUp((0.0, 1.0, 0.0))
    camera.SetClippingRange(1.0, 10.0)
    camera.SetFieldOfView(60.0)
    camera.SetPosition((1.5, 1.5, 1.5))
    return camera


def render_with_mapper(dataset, field_name, mapper, output_name):
    color_table = ColorTable("inferno")
    actor = Actor(dataset, field_name, color_table)
    scene = Scene()
    scene.AddActor(actor)

    canvas = CanvasRayTracer(512, 512)
    view = View3D(scene, mapper, canvas, make_camera(), (0.2, 0.2, 0.2, 1.0))

    with tempfile.TemporaryDirectory() as temp_dir:
        output = Path(temp_dir) / output_name
        view.Paint()
        view.SaveAs(str(output))
        assert output.exists()
        assert output.stat().st_size > 0

