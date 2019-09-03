//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/PointElevation.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

vtkm::cont::DataSet MakePointElevationTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Vec3f_32> coordinates;
  const vtkm::Id dim = 5;
  for (vtkm::Id j = 0; j < dim; ++j)
  {
    vtkm::Float32 z = static_cast<vtkm::Float32>(j) / static_cast<vtkm::Float32>(dim - 1);
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      vtkm::Float32 x = static_cast<vtkm::Float32>(i) / static_cast<vtkm::Float32>(dim - 1);
      vtkm::Float32 y = (x * x + z * z) / 2.0f;
      coordinates.push_back(vtkm::make_Vec(x, y, z));
    }
  }

  vtkm::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, vtkm::CopyFlag::On));

  vtkm::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (vtkm::Id j = 0; j < dim - 1; ++j)
  {
    for (vtkm::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(vtkm::CELL_SHAPE_QUAD,
                      4,
                      vtkm::make_Vec<vtkm::Id>(
                        j * dim + i, j * dim + i + 1, (j + 1) * dim + i + 1, (j + 1) * dim + i));
    }
  }
  cellSet.CompleteAddingCells(vtkm::Id(coordinates.size()));

  dataSet.SetCellSet(cellSet);
  return dataSet;
}
}

void TestPointElevation()
{
  std::cout << "Testing PointElevation Worklet" << std::endl;

  vtkm::cont::DataSet dataSet = MakePointElevationTestDataSet();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  vtkm::worklet::PointElevation pointElevationWorklet;
  pointElevationWorklet.SetLowPoint(vtkm::make_Vec<vtkm::Float64>(0.0, 0.0, 0.0));
  pointElevationWorklet.SetHighPoint(vtkm::make_Vec<vtkm::Float64>(0.0, 1.0, 0.0));
  pointElevationWorklet.SetRange(0.0, 2.0);

  vtkm::worklet::DispatcherMapField<vtkm::worklet::PointElevation> dispatcher(
    pointElevationWorklet);
  dispatcher.Invoke(dataSet.GetCoordinateSystem(), result);

  auto coordinates = dataSet.GetCoordinateSystem().GetData();
  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(test_equal(coordinates.GetPortalConstControl().Get(i)[1] * 2.0,
                                result.GetPortalConstControl().Get(i)),
                     "Wrong result for PointElevation worklet");
  }
}

int UnitTestPointElevation(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPointElevation, argc, argv);
}
