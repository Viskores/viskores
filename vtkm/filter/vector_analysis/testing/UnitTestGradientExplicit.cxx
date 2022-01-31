//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/vector_analysis/Gradient.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestCellGradientExplicit()
{
  std::cout << "Testing Gradient Filter with cell output on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  vtkm::filter::vector_analysis::Gradient gradient;
  gradient.SetOutputFieldName("gradient");
  gradient.SetActiveField("pointvar");

  vtkm::cont::DataSet result = gradient.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasCellField("gradient"), "Result field missing.");

  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> resultArrayHandle;
  result.GetCellField("gradient").GetData().AsArrayHandle(resultArrayHandle);
  vtkm::Vec3f_32 expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, -0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                     "Wrong result for CellGradient filter on 3D explicit data");
  }
}

void TestPointGradientExplicit()
{
  std::cout << "Testing Gradient Filter with point output on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  vtkm::filter::vector_analysis::Gradient gradient;
  gradient.SetComputePointGradient(true);
  gradient.SetOutputFieldName("gradient");
  gradient.SetActiveField("pointvar");

  vtkm::cont::DataSet result = gradient.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasPointField("gradient"), "Result field missing.");

  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> resultArrayHandle;
  result.GetPointField("gradient").GetData().AsArrayHandle(resultArrayHandle);

  vtkm::Vec3f_32 expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, 0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                     "Wrong result for CellGradient filter on 3D explicit data");
  }
}


void TestGradient()
{
  TestCellGradientExplicit();
  TestPointGradientExplicit();
}
}

int UnitTestGradientExplicit(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestGradient, argc, argv);
}
