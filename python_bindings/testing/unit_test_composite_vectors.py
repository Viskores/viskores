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
from viskores.filter.field_transform import CompositeVectors


def main():
    point_ds = viskores.DataSet()
    point_a = np.linspace(0.0, 4.0, 5, dtype=np.float64)
    point_b = np.linspace(10.0, 14.0, 5, dtype=np.float64)
    point_ds.AddPointField("point_a", point_a)
    point_ds.AddPointField("point_b", point_b)

    point_filter = CompositeVectors()
    point_filter.SetFieldNameList(["point_a", "point_b"])
    point_filter.SetOutputFieldName("point_vec")
    point_out = point_filter.Execute(point_ds)
    np.testing.assert_allclose(point_out.GetField("point_vec"), np.stack([point_a, point_b], axis=1))
    assert point_filter.GetNumberOfFields() == 2


if __name__ == "__main__":
    main()
