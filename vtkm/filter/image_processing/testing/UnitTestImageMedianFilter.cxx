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
#include <vtkm/filter/image_processing/ImageMedian.h>

namespace
{

void TestImageMedian()
{
  std::cout << "Testing Image Median Filter on 3D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet2();

  vtkm::filter::image_processing::ImageMedian median;
  median.Perform3x3();
  median.SetActiveField("pointvar");
  auto result = median.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasPointField("median"), "Field missing.");
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  result.GetPointField("median").GetData().AsArrayHandle(resultArrayHandle);

  auto cells = result.GetCellSet().AsCellSet<vtkm::cont::CellSetStructured<3>>();
  auto pdims = cells.GetPointDimensions();

  //verified by hand
  {
    auto portal = resultArrayHandle.ReadPortal();
    vtkm::Float32 expected_median = portal.Get(1 + pdims[0]);
    VTKM_TEST_ASSERT(test_equal(expected_median, 2), "incorrect median value");

    expected_median = portal.Get(1 + pdims[0] + (pdims[1] * pdims[0] * 2));
    VTKM_TEST_ASSERT(test_equal(expected_median, 2.82843), "incorrect median value");
  }
}
}

int UnitTestImageMedianFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestImageMedian, argc, argv);
}
