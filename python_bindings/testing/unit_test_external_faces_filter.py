##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.filter.clean_grid import CleanGrid
from viskores.filter.entity_extraction import ExternalFaces
from viskores.testing import MakeTestDataSet


def make_data_test_set1():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    clean = CleanGrid()
    clean.SetCompactPointFields(False)
    clean.SetMergePoints(False)
    return clean.Execute(dataset)


def check_external_faces(dataset, compact_points, expected_cells, expected_points=None, pass_poly_data=True):
    external_faces = ExternalFaces()
    external_faces.SetCompactPoints(compact_points)
    external_faces.SetPassPolyData(pass_poly_data)
    result = external_faces.Execute(dataset)

    assert result.GetNumberOfCells() == expected_cells
    assert result.HasField("pointvar")
    assert result.HasField("cellvar")

    if compact_points and expected_points is not None:
        assert result.GetNumberOfPoints() == expected_points


def test_hexahedra_mesh():
    dataset = make_data_test_set1()
    check_external_faces(dataset, False, 96)
    check_external_faces(dataset, True, 96, 98)


def test_heterogeneous_mesh():
    dataset = MakeTestDataSet().Make3DExplicitDataSet5()
    check_external_faces(dataset, False, 12)
    check_external_faces(dataset, True, 12, 11)


def test_uniform_mesh():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    check_external_faces(dataset, False, 16 * 6)
    check_external_faces(dataset, True, 16 * 6, 98)


def test_rectilinear_mesh():
    dataset = MakeTestDataSet().Make3DRectilinearDataSet0()
    check_external_faces(dataset, False, 16)
    check_external_faces(dataset, True, 16, 18)


def test_mixed_2d_and_3d_mesh():
    dataset = MakeTestDataSet().Make3DExplicitDataSet6()
    check_external_faces(dataset, False, 12)
    check_external_faces(dataset, True, 12, 8)
    check_external_faces(dataset, False, 6, pass_poly_data=False)
    check_external_faces(dataset, True, 6, 5, pass_poly_data=False)


def main():
    test_hexahedra_mesh()
    test_heterogeneous_mesh()
    test_uniform_mesh()
    test_rectilinear_mesh()
    test_mixed_2d_and_3d_mesh()


if __name__ == "__main__":
    main()
