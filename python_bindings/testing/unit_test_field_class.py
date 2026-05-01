##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

import numpy as np

from viskores.cont import Field, UnknownArrayHandle, make_FieldCell, make_FieldPoint
from viskores.python_convenience import array_from_numpy


def main():
    values = np.array([1.0, 2.0, 3.0], dtype=np.float32)
    field = Field("values", Field.Association.Points, array_from_numpy(values))
    assert field.GetName() == "values"
    assert field.GetAssociation() == Field.Association.Points
    assert field.GetNumberOfValues() == 3
    assert field.IsPointField()
    assert not field.IsCellField()
    assert isinstance(field.GetData(), UnknownArrayHandle)
    np.testing.assert_allclose(field.GetData().AsNumPy(), values)
    field_data = field.GetData()
    new_values = np.array([4.0, 5.0, 6.0], dtype=np.float32)
    field.SetData(array_from_numpy(new_values))
    np.testing.assert_allclose(field_data.AsNumPy(), new_values)

    point_field = make_FieldPoint("point_values", values)
    assert point_field.IsPointField()
    np.testing.assert_allclose(point_field.GetData().AsNumPy(), values)

    cell_values = np.array([10.0, 20.0], dtype=np.float32)
    cell_field = make_FieldCell("cell_values", cell_values)
    assert cell_field.IsCellField()
    np.testing.assert_allclose(cell_field.GetData().AsNumPy(), cell_values)


if __name__ == "__main__":
    main()
