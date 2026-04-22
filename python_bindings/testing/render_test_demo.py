##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from pathlib import Path

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
    out_dir = Path.cwd()
    outputs = [
        out_dir / "volume.png",
        out_dir / "isosurface_wireframer.png",
        out_dir / "isosurface_raytracer.png",
    ]

    tangle = Tangle()
    tangle.SetPointDimensions((8, 8, 8))
    tangle_data = tangle.Execute()
    field_name = "tangle"

    camera = Camera()
    camera.SetLookAt((0.5, 0.5, 0.5))
    camera.SetViewUp((0.0, 1.0, 0.0))
    camera.SetClippingRange(1.0, 10.0)
    camera.SetFieldOfView(60.0)
    camera.SetPosition((1.5, 1.5, 1.5))

    color_table = ColorTable("inferno")
    background = (0.2, 0.2, 0.2, 1.0)

    actor = Actor(tangle_data, field_name, color_table)
    scene = Scene()
    scene.AddActor(actor)
    canvas = CanvasRayTracer(256, 256)

    view = View3D(scene, MapperVolume(), canvas, camera, background)
    view.Paint()
    view.SaveAs(str(outputs[0]))

    contour = Contour()
    contour.SetIsoValue(3.0)
    contour.SetActiveField(field_name)
    iso_data = contour.Execute(tangle_data)

    iso_actor = Actor(iso_data, field_name, color_table)
    iso_actor.SetScalarRange(actor.GetScalarRange())
    iso_scene = Scene()
    iso_scene.AddActor(iso_actor)

    iso_view = View3D(iso_scene, MapperWireframer(), canvas, camera, background)
    iso_view.Paint()
    iso_view.SaveAs(str(outputs[1]))

    solid_view = View3D(iso_scene, MapperRayTracer(), canvas, camera, background)
    solid_view.Paint()
    solid_view.SaveAs(str(outputs[2]))

    for path in outputs:
        assert path.is_file(), f"Expected render output {path}."
        assert path.stat().st_size > 0, f"Expected non-empty render output {path}."


if __name__ == "__main__":
    main()
