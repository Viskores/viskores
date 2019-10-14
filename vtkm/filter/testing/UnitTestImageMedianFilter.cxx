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
#include <vtkm/filter/ImageMedian.h>

namespace
{

void TestImageMedian()
{
  std::cout << "Testing Image Median Filter on 3D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet2();

  vtkm::filter::ImageMedian median;
  median.Perform3x3();
  median.SetActiveField("pointvar");
  auto result = median.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasPointField("median"), "Field missing.");
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  result.GetPointField("median").GetData().CopyTo(resultArrayHandle);

  auto cells = result.GetCellSet().Cast<vtkm::cont::CellSetStructured<3>>();
  auto pdims = cells.GetPointDimensions();

  //verified by hand
  {
    auto portal = resultArrayHandle.GetPortalConstControl();
    std::cout << "spot to verify x = 1, y = 1, z = 0 is: ";
    vtkm::Float32 temp = portal.Get(1 + pdims[0]);
    std::cout << temp << std::endl << std::endl;
    VTKM_TEST_ASSERT(test_equal(temp, 2), "incorrect median value");

    std::cout << "spot to verify x = 1, y = 1, z = 2 is: ";
    temp = portal.Get(1 + pdims[0] + (pdims[1] * pdims[0] * 2));
    std::cout << temp << std::endl << std::endl;
    VTKM_TEST_ASSERT(test_equal(temp, 2.82843), "incorrect median value");
  }
}
}

int UnitTestImageMedianFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestImageMedian, argc, argv);
}
