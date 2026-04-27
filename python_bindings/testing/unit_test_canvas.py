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
from viskores.rendering import Canvas


def main():
    canvas = Canvas()
    canvas.SetBackgroundColor((1.0, 1.0, 1.0, 1.0))
    canvas.Clear()
    canvas.AddLine(-0.8, 0.8, 0.8, 0.8, 1.0, (0.0, 0.0, 0.0, 1.0))
    canvas.AddLine(0.8, 0.8, 0.8, -0.8, 1.0, (0.0, 0.0, 0.0, 1.0))
    canvas.AddLine(0.8, -0.8, -0.8, -0.8, 1.0, (0.0, 0.0, 0.0, 1.0))
    canvas.AddLine(-0.8, -0.8, -0.8, 0.8, 1.0, (0.0, 0.0, 0.0, 1.0))
    canvas.AddLine(-0.8, -0.8, 0.8, 0.8, 1.0, (0.0, 0.0, 0.0, 1.0))
    canvas.AddLine(-0.8, 0.8, 0.8, -0.8, 1.0, (0.0, 0.0, 0.0, 1.0))
    canvas.AddColorBar((-0.8, -0.6, -0.8, 0.8, 0.0, 0.0), ColorTable("inferno"), False)
    canvas.BlendBackground()

    with tempfile.TemporaryDirectory() as temp_dir:
        output = Path(temp_dir) / "canvas.pnm"
        canvas.SaveAs(str(output))
        assert output.exists()
        assert output.stat().st_size > 0


if __name__ == "__main__":
    main()
