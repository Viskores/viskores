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
#include <vtkm/worklet/WarpVector.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace
{
template <typename T>
vtkm::cont::DataSet MakeWarpVectorTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Vec<T, 3>> coordinates;
  const vtkm::Id dim = 5;
  for (vtkm::Id j = 0; j < dim; ++j)
  {
    T z = static_cast<T>(j) / static_cast<T>(dim - 1);
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      T x = static_cast<T>(i) / static_cast<T>(dim - 1);
      T y = (x * x + z * z) / 2.0f;
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

void TestWarpVector()
{
  std::cout << "Testing WarpVector Worklet" << std::endl;
  using vecType = vtkm::Vec3f;

  vtkm::cont::DataSet ds = MakeWarpVectorTestDataSet<vtkm::FloatDefault>();
  vtkm::cont::ArrayHandle<vecType> result;

  vtkm::FloatDefault scale = 2;

  vecType vector = vtkm::make_Vec<vtkm::FloatDefault>(static_cast<vtkm::FloatDefault>(0.0),
                                                      static_cast<vtkm::FloatDefault>(0.0),
                                                      static_cast<vtkm::FloatDefault>(2.0));
  auto coordinate = ds.GetCoordinateSystem().GetData();
  vtkm::Id nov = coordinate.GetNumberOfValues();
  vtkm::cont::ArrayHandleConstant<vecType> vectorAH =
    vtkm::cont::make_ArrayHandleConstant(vector, nov);

  vtkm::worklet::WarpVector warpWorklet;
  warpWorklet.Run(ds.GetCoordinateSystem(), vectorAH, scale, result);
  auto resultPortal = result.GetPortalConstControl();
  for (vtkm::Id i = 0; i < nov; i++)
  {
    for (vtkm::Id j = 0; j < 3; j++)
    {
      vtkm::FloatDefault ans =
        coordinate.GetPortalConstControl().Get(i)[static_cast<vtkm::IdComponent>(j)] +
        scale * vector[static_cast<vtkm::IdComponent>(j)];
      VTKM_TEST_ASSERT(test_equal(ans, resultPortal.Get(i)[static_cast<vtkm::IdComponent>(j)]),
                       " Wrong result for WarpVector worklet");
    }
  }
}

int UnitTestWarpVector(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestWarpVector, argc, argv);
}
