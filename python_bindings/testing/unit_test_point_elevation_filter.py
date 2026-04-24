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

import viskores
from viskores.filter.field_transform import PointElevation


def main():
    dims = (5, 5, 5)
    spacing = (0.25, 0.25, 0.25)
    ds = viskores.create_uniform_dataset(dims, spacing=spacing)

    filt = PointElevation()
    filt.SetLowPoint((0.0, 0.0, 0.0))
    filt.SetHighPoint((0.0, 1.0, 0.0))
    filt.SetRange(0.0, 2.0)
    filt.SetUseCoordinateSystemAsField(True)
    filt.SetOutputFieldName("height")

    out = filt.Execute(ds)
    y_coords = np.arange(dims[1], dtype=np.float64) * spacing[1]
    expected = np.tile(np.repeat(y_coords * 2.0, dims[0]), dims[2])
    np.testing.assert_allclose(out.GetField("height"), expected)


if __name__ == "__main__":
    main()

