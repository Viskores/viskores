##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

import numpy as np

import viskores
import viskores.cont


# ----- Field and FieldAssociation -------------------------------------------

def check_field_association_values():
    assoc = viskores.cont.FieldAssociation
    assert assoc.Points != assoc.Cells
    assert assoc.Points != assoc.WholeDataSet
    assert assoc.Points != assoc.Any


def check_field_construction():
    data = np.arange(12, dtype=np.float32)
    unknown = viskores.cont.array_from_numpy(data)
    field = viskores.cont.Field("pressure", viskores.cont.FieldAssociation.Points, unknown)
    assert field.GetName() == "pressure"
    assert field.GetAssociation() == viskores.cont.FieldAssociation.Points
    assert field.GetNumberOfValues() == 12
    np.testing.assert_array_equal(field.asnumpy(), data)


# ----- DataSetBuilderUniform ------------------------------------------------

def check_uniform_dataset_1d():
    ds = viskores.cont.DataSetBuilderUniform.create([5])
    assert ds.GetNumberOfPoints() == 5
    assert ds.GetNumberOfCells() == 4
    assert ds.GetNumberOfCoordinateSystems() == 1


def check_uniform_dataset_2d():
    ds = viskores.cont.DataSetBuilderUniform.create([4, 5])
    assert ds.GetNumberOfPoints() == 20
    assert ds.GetNumberOfCells() == 12
    assert ds.GetNumberOfCoordinateSystems() == 1


def check_uniform_dataset_3d():
    ds = viskores.cont.DataSetBuilderUniform.create([3, 4, 5])
    assert ds.GetNumberOfPoints() == 60
    assert ds.GetNumberOfCells() == 24
    assert ds.GetNumberOfCoordinateSystems() == 1


def check_uniform_dataset_rejects_bad_dims():
    try:
        viskores.cont.DataSetBuilderUniform.create([])
    except RuntimeError:
        pass
    else:
        raise AssertionError("Expected empty dimension list to be rejected.")

    try:
        viskores.cont.DataSetBuilderUniform.create([2, 3, 4, 5])
    except RuntimeError:
        pass
    else:
        raise AssertionError("Expected 4-element dimension list to be rejected.")


# ----- DataSet field round-trip ---------------------------------------------

def check_dataset_point_field_roundtrip():
    ds = viskores.cont.DataSetBuilderUniform.create([3, 4])
    npts = ds.GetNumberOfPoints()
    assert npts == 12

    data = np.linspace(0.0, 1.0, npts, dtype=np.float32)
    ds.AddPointField("temperature", viskores.cont.array_from_numpy(data, allow_copy=True))

    assert ds.HasField("temperature", viskores.cont.FieldAssociation.Points)
    assert ds.GetNumberOfFields() >= 1

    field = ds.GetField("temperature", viskores.cont.FieldAssociation.Points)
    assert field.GetName() == "temperature"
    assert field.GetAssociation() == viskores.cont.FieldAssociation.Points
    np.testing.assert_array_almost_equal(field.asnumpy(), data)


def check_dataset_cell_field_roundtrip():
    ds = viskores.cont.DataSetBuilderUniform.create([3, 4])
    ncells = ds.GetNumberOfCells()
    assert ncells == 6

    data = np.arange(ncells, dtype=np.float64)
    ds.AddCellField("pressure", viskores.cont.array_from_numpy(data, allow_copy=True))

    assert ds.HasField("pressure", viskores.cont.FieldAssociation.Cells)

    field = ds.GetField("pressure", viskores.cont.FieldAssociation.Cells)
    assert field.GetName() == "pressure"
    assert field.GetAssociation() == viskores.cont.FieldAssociation.Cells
    np.testing.assert_array_equal(field.asnumpy(), data)


def check_dataset_field_object():
    ds = viskores.cont.DataSetBuilderUniform.create([5])
    npts = ds.GetNumberOfPoints()

    data = np.arange(npts, dtype=np.int32)
    unknown = viskores.cont.array_from_numpy(data, allow_copy=True)
    field = viskores.cont.Field("ids", viskores.cont.FieldAssociation.Points, unknown)
    ds.AddField(field)

    retrieved = ds.GetField("ids", viskores.cont.FieldAssociation.Points)
    np.testing.assert_array_equal(retrieved.asnumpy(), data)


def check_dataset_has_field():
    ds = viskores.cont.DataSetBuilderUniform.create([5])
    npts = ds.GetNumberOfPoints()

    data = np.zeros(npts, dtype=np.float32)
    ds.AddPointField("u", viskores.cont.array_from_numpy(data, allow_copy=True))

    assert ds.HasField("u", viskores.cont.FieldAssociation.Points)
    assert not ds.HasField("u", viskores.cont.FieldAssociation.Cells)
    assert not ds.HasField("missing")


# ----- CoordinateSystem from NumPy ------------------------------------------

def check_coordinate_system_from_numpy():
    coords = np.array([[0.0, 0.0, 0.0],
                       [1.0, 0.0, 0.0],
                       [1.0, 1.0, 0.0],
                       [0.0, 1.0, 0.0]], dtype=np.float32)
    unknown = viskores.cont.array_from_numpy(coords, allow_copy=True)
    cs = viskores.cont.CoordinateSystem("pts", unknown)
    assert cs.GetName() == "pts"
    assert cs.GetNumberOfValues() == 4

    bounds = cs.GetBounds()
    assert len(bounds) == 6
    assert bounds[0] == 0.0 and bounds[1] == 1.0  # X
    assert bounds[2] == 0.0 and bounds[3] == 1.0  # Y
    assert bounds[4] == 0.0 and bounds[5] == 0.0  # Z


def check_coordinate_system_from_numpy_float64():
    coords = np.array([[0.0, 0.0, 0.0],
                       [2.0, 3.0, 4.0]], dtype=np.float64)
    unknown = viskores.cont.array_from_numpy(coords, allow_copy=True)
    cs = viskores.cont.CoordinateSystem("pts", unknown)
    bounds = cs.GetBounds()
    assert bounds[0] == 0.0 and bounds[1] == 2.0
    assert bounds[2] == 0.0 and bounds[3] == 3.0
    assert bounds[4] == 0.0 and bounds[5] == 4.0


def check_coordinate_system_asnumpy_roundtrip_float32():
    coords = np.array([[0.0, 0.0, 0.0],
                       [1.0, 2.0, 3.0],
                       [4.0, 5.0, 6.0]], dtype=np.float32)
    unknown = viskores.cont.array_from_numpy(coords, allow_copy=True)
    cs = viskores.cont.CoordinateSystem("pts", unknown)
    result = cs.asnumpy()
    np.testing.assert_array_almost_equal(result, coords)


def check_coordinate_system_asnumpy_roundtrip_float64():
    coords = np.array([[0.0, 0.0, 0.0],
                       [1.0, 2.0, 3.0],
                       [4.0, 5.0, 6.0]], dtype=np.float64)
    unknown = viskores.cont.array_from_numpy(coords, allow_copy=True)
    cs = viskores.cont.CoordinateSystem("pts", unknown)
    result = cs.asnumpy()
    np.testing.assert_array_almost_equal(result, coords)


def main():
    check_field_association_values()
    check_field_construction()

    check_uniform_dataset_1d()
    check_uniform_dataset_2d()
    check_uniform_dataset_3d()
    check_uniform_dataset_rejects_bad_dims()

    check_dataset_point_field_roundtrip()
    check_dataset_cell_field_roundtrip()
    check_dataset_field_object()
    check_dataset_has_field()

    check_coordinate_system_from_numpy()
    check_coordinate_system_from_numpy_float64()

    check_coordinate_system_asnumpy_roundtrip_float32()
    check_coordinate_system_asnumpy_roundtrip_float64()


if __name__ == "__main__":
    main()
