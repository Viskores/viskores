//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/geometry_refinement/ConvertToPointCloud.h>

#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

void CheckPointCloudCells(const vtkm::cont::UnknownCellSet& cellSet, vtkm::Id numPoints)
{
  // A point cloud has the same number of cells as points. All cells are vertex
  // cells with one point. That point index is the same as the cell index.

  VTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == numPoints);
  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == numPoints);

  for (vtkm::Id index = 0; index < numPoints; ++index)
  {
    VTKM_TEST_ASSERT(cellSet.GetCellShape(index) == vtkm::CELL_SHAPE_VERTEX);
    VTKM_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(index) == 1);

    vtkm::Id pointId;
    cellSet.GetCellPointIds(index, &pointId);
    VTKM_TEST_ASSERT(pointId == index);
  }
}

void CheckPointCloudCells(const vtkm::cont::DataSet& dataSet, vtkm::Id numPoints)
{
  CheckPointCloudCells(dataSet.GetCellSet(), numPoints);
}

void TryConvertToPointCloud(const vtkm::cont::DataSet& dataSet)
{
  {
    std::cout << "  convert to point cloud" << std::endl;
    vtkm::filter::geometry_refinement::ConvertToPointCloud convertFilter;
    vtkm::cont::DataSet pointCloud = convertFilter.Execute(dataSet);
    CheckPointCloudCells(pointCloud, dataSet.GetNumberOfPoints());

    for (vtkm::IdComponent coordId = 0; coordId < dataSet.GetNumberOfCoordinateSystems(); ++coordId)
    {
      const auto& coords = dataSet.GetCoordinateSystem(coordId);
      std::cout << "    coord system " << coords.GetName() << std::endl;
      VTKM_TEST_ASSERT(pointCloud.HasCoordinateSystem(coords.GetName()));
    }

    for (vtkm::IdComponent fieldId = 0; fieldId < dataSet.GetNumberOfFields(); ++fieldId)
    {
      const auto& field = dataSet.GetField(fieldId);
      std::cout << "    field " << field.GetName() << std::endl;
      switch (field.GetAssociation())
      {
        case vtkm::cont::Field::Association::Cells:
          VTKM_TEST_ASSERT(!pointCloud.HasField(field.GetName()));
          break;
        default:
          VTKM_TEST_ASSERT(pointCloud.HasField(field.GetName(), field.GetAssociation()));
          break;
      }
    }
  }

  {
    std::cout << "  convert to point cloud with cell data" << std::endl;
    vtkm::filter::geometry_refinement::ConvertToPointCloud convertFilter;
    convertFilter.SetAssociateFieldsWithCells(true);
    vtkm::cont::DataSet pointCloud = convertFilter.Execute(dataSet);
    CheckPointCloudCells(pointCloud, dataSet.GetNumberOfPoints());

    for (vtkm::IdComponent coordId = 0; coordId < dataSet.GetNumberOfCoordinateSystems(); ++coordId)
    {
      const auto& coords = dataSet.GetCoordinateSystem(coordId);
      std::cout << "    coord system " << coords.GetName() << std::endl;
      VTKM_TEST_ASSERT(pointCloud.HasCoordinateSystem(coords.GetName()));
    }

    for (vtkm::IdComponent fieldId = 0; fieldId < dataSet.GetNumberOfFields(); ++fieldId)
    {
      auto& field = dataSet.GetField(fieldId);
      std::cout << "    field " << field.GetName() << std::endl;
      switch (field.GetAssociation())
      {
        case vtkm::cont::Field::Association::Cells:
          VTKM_TEST_ASSERT(!pointCloud.HasField(field.GetName()));
          break;
        case vtkm::cont::Field::Association::Points:
        {
          auto correctAssociation = dataSet.HasCoordinateSystem(field.GetName())
            ? vtkm::cont::Field::Association::Points
            : vtkm::cont::Field::Association::Cells;
          VTKM_TEST_ASSERT(pointCloud.HasField(field.GetName(), correctAssociation));
        }
        break;
        default:
          VTKM_TEST_ASSERT(pointCloud.HasField(field.GetName(), field.GetAssociation()));
          break;
      }
    }
  }
}

void TryFile(const std::string& filename)
{
  std::cout << "Testing " << filename << std::endl;
  std::string fullpath = vtkm::cont::testing::Testing::DataPath(filename);
  vtkm::io::VTKDataSetReader reader(fullpath);
  TryConvertToPointCloud(reader.ReadDataSet());
}

void Run()
{
  TryFile("uniform/simple_structured_points_bin.vtk");
  TryFile("rectilinear/DoubleGyre_0.vtk");
  TryFile("curvilinear/kitchen.vtk");
  TryFile("unstructured/simple_unstructured_bin.vtk");
}

} // anonymous namespace

int UnitTestConvertToPointCloud(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
