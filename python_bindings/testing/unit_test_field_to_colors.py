##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import numpy as np

from viskores.cont import ColorTable
from viskores.filter.field_transform import FieldToColors
from viskores.testing import MakeTestDataSet


def main():
    data = np.asarray([-1, 0, 10, 20, 30, 40, 50, 60], dtype=np.float32)

    table = ColorTable("Cool to Warm")
    table.RescaleToRange((0.0, 50.0))
    table.SetClampingOff()
    table.SetAboveRangeColor((1.0, 0.0, 0.0, 1.0))
    table.SetBelowRangeColor((0.0, 0.0, 1.0, 1.0))

    ds = MakeTestDataSet().Make3DExplicitDataSetPolygonal()
    ds.AddPointField("faux", data)

    ftc = FieldToColors(table)
    ftc.SetActiveField("faux")
    ftc.SetOutputFieldName("colors")
    ftc.SetOutputToRGBA()
    rgba = ftc.Execute(ds).GetField("colors")

    expected_rgba = np.asarray(
        [
            [0, 0, 255, 255],
            [59, 76, 192, 255],
            [124, 159, 249, 255],
            [192, 212, 245, 255],
            [242, 203, 183, 255],
            [238, 133, 104, 255],
            [180, 4, 38, 255],
            [255, 0, 0, 255],
        ],
        dtype=np.uint8,
    )
    np.testing.assert_array_equal(rgba, expected_rgba)

    ftc.SetOutputToRGB()
    rgb = ftc.Execute(ds).GetField("colors")
    np.testing.assert_array_equal(rgb, expected_rgba[:, :3])


if __name__ == "__main__":
    main()
