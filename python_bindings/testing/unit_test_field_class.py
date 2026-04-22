##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

import numpy as np

from viskores.cont import Association, Field, make_FieldCell, make_FieldPoint


def main():
    values = np.array([1.0, 2.0, 3.0], dtype=np.float32)
    field = Field("values", Association.POINTS, values)
    assert field.GetName() == "values"
    assert field.GetAssociation() == Association.POINTS
    assert field.GetNumberOfValues() == 3
    assert field.IsPointField()
    assert not field.IsCellField()
    np.testing.assert_allclose(field.GetData(), values)

    point_field = make_FieldPoint("point_values", values)
    assert point_field.IsPointField()
    np.testing.assert_allclose(point_field.GetData(), values)

    cell_values = np.array([10.0, 20.0], dtype=np.float32)
    cell_field = make_FieldCell("cell_values", cell_values)
    assert cell_field.IsCellField()
    np.testing.assert_allclose(cell_field.GetData(), cell_values)


if __name__ == "__main__":
    main()
