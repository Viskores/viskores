//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/CellAverage.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestCellAverageUniform3D()
{
  std::cout << "Testing CellAverage Worklet on 3D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(), dataSet.GetField("pointvar"), result);

  vtkm::Float32 expected[4] = { 60.1875f, 70.2125f, 120.3375f, 130.3625f };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 3D uniform data");
  }
}

void TestCellAverageUniform2D()
{
  std::cout << "Testing CellAverage Worklet on 2D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(), dataSet.GetField("pointvar"), result);

  vtkm::Float32 expected[2] = { 30.1f, 40.1f };
  std::cout << std::endl;
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 2D uniform data");
  }
}

void TestCellAverageExplicit()
{
  std::cout << "Testing CellAverage Worklet on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(), dataSet.GetField("pointvar"), result);

  vtkm::Float32 expected[2] = { 20.1333f, 35.2f };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 3D explicit data");
  }
}

void TestCellAverage()
{
  TestCellAverageUniform2D();
  TestCellAverageUniform3D();
  TestCellAverageExplicit();
}
}

int UnitTestCellAverage(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellAverage, argc, argv);
}
