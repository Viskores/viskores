##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from pathlib import Path

from viskores.filter.geometry_refinement import ConvertToPointCloud
from viskores.io import VTKDataSetReader


def check_dataset(dataset):
    convert_filter = ConvertToPointCloud()
    point_cloud = convert_filter.Execute(dataset)

    num_points = dataset.GetNumberOfPoints()
    assert point_cloud.GetNumberOfPoints() == num_points
    assert point_cloud.GetNumberOfCells() == num_points

    for coord_id in range(dataset.GetNumberOfCoordinateSystems()):
        coord_name = dataset.GetCoordinateSystemName(coord_id)
        assert point_cloud.HasCoordinateSystem(coord_name)

    for field_name in dataset.FieldNames():
        if dataset.HasField(field_name, association="cells"):
            assert not point_cloud.HasField(field_name)
        elif dataset.HasField(field_name, association="points"):
            assert point_cloud.HasField(field_name, association="points")
        elif dataset.HasField(field_name, association="whole_dataset"):
            assert point_cloud.HasField(field_name, association="whole_dataset")
        else:
            assert point_cloud.HasField(field_name)

    convert_filter = ConvertToPointCloud()
    convert_filter.SetAssociateFieldsWithCells(True)
    point_cloud = convert_filter.Execute(dataset)

    assert point_cloud.GetNumberOfPoints() == num_points
    assert point_cloud.GetNumberOfCells() == num_points

    for coord_id in range(dataset.GetNumberOfCoordinateSystems()):
        coord_name = dataset.GetCoordinateSystemName(coord_id)
        assert point_cloud.HasCoordinateSystem(coord_name)

    for field_name in dataset.FieldNames():
        if dataset.HasField(field_name, association="cells"):
            assert not point_cloud.HasField(field_name)
        elif dataset.HasField(field_name, association="points"):
            if dataset.HasCoordinateSystem(field_name):
                assert point_cloud.HasField(field_name, association="points")
            else:
                assert point_cloud.HasField(field_name, association="cells")
        elif dataset.HasField(field_name, association="whole_dataset"):
            assert point_cloud.HasField(field_name, association="whole_dataset")
        else:
            assert point_cloud.HasField(field_name)


def main():
    repo_root = Path(__file__).resolve().parents[2]
    data_files = (
        "data/data/uniform/simple_structured_points_bin.vtk",
        "data/data/rectilinear/DoubleGyre_0.vtk",
        "data/data/curvilinear/kitchen.vtk",
        "data/data/unstructured/simple_unstructured_bin.vtk",
    )

    for relative_path in data_files:
        dataset = VTKDataSetReader(str(repo_root / relative_path)).ReadDataSet()
        check_dataset(dataset)


if __name__ == "__main__":
    main()
