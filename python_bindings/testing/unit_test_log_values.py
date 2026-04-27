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
from viskores.filter.field_transform import LogValues


def check_log(filter_obj, field_name, expected):
    filter_obj.SetActiveField(field_name)
    filter_obj.SetOutputFieldName(f"{field_name}_log")
    out = filter_obj.Execute(dataset)
    np.testing.assert_allclose(out.GetField(f"{field_name}_log"), expected, rtol=1e-6, atol=1e-7)


dataset = viskores.DataSet()
point_values = np.linspace(0.0, 9.9, 100, dtype=np.float64)
cell_values = np.linspace(0.0, 9.9, 100, dtype=np.float64)
dataset.AddPointField("pointScalarField", point_values)
dataset.AddCellField("cellScalarField", cell_values)


def main():
    filt = LogValues()
    min_value = np.finfo(np.float32).tiny
    expected_e = np.log(np.where(point_values == 0.0, min_value, point_values))
    check_log(filt, "pointScalarField", expected_e)

    filt = LogValues()
    filt.SetBaseValueTo2()
    expected_2 = np.log2(np.where(point_values == 0.0, min_value, point_values))
    check_log(filt, "pointScalarField", expected_2)

    filt = LogValues()
    filt.SetBaseValueTo10()
    expected_10 = np.log10(np.where(cell_values == 0.0, min_value, cell_values))
    check_log(filt, "cellScalarField", expected_10)


if __name__ == "__main__":
    main()
