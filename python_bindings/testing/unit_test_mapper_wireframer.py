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
from viskores.filter.contour import Contour
from viskores.rendering import (
    Actor,
    Camera,
    CanvasRayTracer,
    MapperWireframer,
    Scene,
    View3D,
)
from viskores.source import Tangle


def main():
    tangle = Tangle()
    tangle.SetPointDimensions((25, 25, 25))
    dataset = tangle.Execute()

    contour = Contour()
    contour.SetActiveField("tangle")
    contour.SetIsoValue(3.0)
    contour_data = contour.Execute(dataset)

    color_table = ColorTable("inferno")
    actor = Actor(contour_data, "tangle", color_table)
    scene = Scene()
    scene.AddActor(actor)

    camera = Camera()
    camera.SetLookAt((0.5, 0.5, 0.5))
    camera.SetViewUp((0.0, 1.0, 0.0))
    camera.SetClippingRange(1.0, 10.0)
    camera.SetFieldOfView(60.0)
    camera.SetPosition((1.5, 1.5, 1.5))

    canvas = CanvasRayTracer(512, 512)
    view = View3D(scene, MapperWireframer(), canvas, camera, (0.2, 0.2, 0.2, 1.0))

    with tempfile.TemporaryDirectory() as temp_dir:
        output = Path(temp_dir) / "wireframer.png"
        view.Paint()
        view.SaveAs(str(output))
        assert output.exists()
        assert output.stat().st_size > 0


if __name__ == "__main__":
    main()
