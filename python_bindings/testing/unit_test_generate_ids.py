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
from viskores.filter.field_transform import GenerateIds


def main():
    ds = viskores.create_uniform_dataset((4, 4, 4))

    filt = GenerateIds()
    out = filt.Execute(ds)
    np.testing.assert_array_equal(out.GetField("pointids"), np.arange(ds.GetNumberOfPoints()))
    np.testing.assert_array_equal(out.GetField("cellids"), np.arange(ds.GetNumberOfCells()))

    float_filter = GenerateIds()
    float_filter.SetUseFloat(True)
    out_float = float_filter.Execute(ds)
    np.testing.assert_allclose(
        out_float.GetField("pointids"), np.arange(ds.GetNumberOfPoints(), dtype=np.float64)
    )

    point_only = GenerateIds()
    point_only.SetGenerateCellIds(False)
    point_only.SetPointFieldName("indices")
    out_point_only = point_only.Execute(ds)
    np.testing.assert_array_equal(
        out_point_only.GetField("indices"), np.arange(ds.GetNumberOfPoints())
    )

    cell_only = GenerateIds()
    cell_only.SetGeneratePointIds(False)
    cell_only.SetCellFieldName("cell_indices")
    out_cell_only = cell_only.Execute(ds)
    np.testing.assert_array_equal(
        out_cell_only.GetField("cell_indices"), np.arange(ds.GetNumberOfCells())
    )


if __name__ == "__main__":
    main()

