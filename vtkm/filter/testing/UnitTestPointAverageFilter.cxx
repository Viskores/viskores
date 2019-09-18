//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/PointAverage.h>

namespace
{

void TestPointAverageUniform3D()
{
  std::cout << "Testing PointAverage Filter on 3D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::filter::PointAverage pointAverage;
  pointAverage.SetOutputFieldName("avgvals");
  pointAverage.SetActiveField("cellvar");
  auto result = pointAverage.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasPointField("avgvals"), "Field missing.");
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  result.GetPointField("avgvals").GetData().CopyTo(resultArrayHandle);
  vtkm::Float32 expected[18] = { 100.1f, 100.15f, 100.2f, 100.1f, 100.15f, 100.2f,
                                 100.2f, 100.25f, 100.3f, 100.2f, 100.25f, 100.3f,
                                 100.3f, 100.35f, 100.4f, 100.3f, 100.35f, 100.4f };
  for (vtkm::Id i = 0; i < 18; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointAverage worklet on 3D regular data");
  }
}

void TestPointAverageRegular3D()
{
  std::cout << "Testing PointAverage Filter on 2D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DRectilinearDataSet0();

  vtkm::filter::PointAverage pointAverage;
  pointAverage.SetActiveField("cellvar");
  auto result = pointAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VTKM_TEST_ASSERT(result.HasPointField("cellvar"), "Field missing.");
  vtkm::cont::Field Result = result.GetPointField("cellvar");
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  Result.GetData().CopyTo(resultArrayHandle);

  vtkm::Float32 expected[18] = { 0.f, 0.5f, 1.f, 0.f, 0.5f, 1.f, 1.f, 1.5f, 2.f,
                                 1.f, 1.5f, 2.f, 2.f, 2.5f, 3.f, 2.f, 2.5f, 3.f };
  for (vtkm::Id i = 0; i < 18; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointAverage worklet on 3D regular data");
  }
}

void TestPointAverageExplicit1()
{
  std::cout << "Testing PointAverage Filter on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet1();

  vtkm::filter::PointAverage pointAverage;
  pointAverage.SetActiveField("cellvar");
  auto result = pointAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VTKM_TEST_ASSERT(result.HasPointField("cellvar"), "Field missing.");
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  result.GetPointField("cellvar").GetData().CopyTo(resultArrayHandle);
  vtkm::Float32 expected[5] = { 100.1f, 100.15f, 100.15f, 100.2f, 100.2f };
  for (int i = 0; i < 5; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointAverage worklet on 3D regular data");
  }
}

void TestPointAverageExplicit2()
{
  std::cout << "Testing PointAverage Filter on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet5();

  vtkm::filter::PointAverage pointAverage;
  pointAverage.SetActiveField("cellvar");
  auto result = pointAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VTKM_TEST_ASSERT(result.HasPointField("cellvar"), "Field missing.");
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  result.GetPointField("cellvar").GetData().CopyTo(resultArrayHandle);
  vtkm::Float32 expected[11] = { 100.1f, 105.05f, 105.05f, 100.1f, 115.3f, 115.2f,
                                 115.2f, 115.3f,  115.1f,  130.5f, 125.35f };
  for (int i = 0; i < 11; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointAverage worklet on 3D regular data");
  }
}

void TestPointAverage()
{
  TestPointAverageUniform3D();
  TestPointAverageRegular3D();
  TestPointAverageExplicit1();
  TestPointAverageExplicit2();
}
}

int UnitTestPointAverageFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPointAverage, argc, argv);
}
