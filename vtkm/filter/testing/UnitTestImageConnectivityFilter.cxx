//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/ImageConnectivity.h>

#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

vtkm::cont::DataSet MakeTestDataSet()
{
  // example from Figure 35.7 of Connected Component Labeling in CUDA by OndˇrejˇŚtava,
  // Bedˇrich Beneˇ
  std::vector<vtkm::UInt8> pixels{
    0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0,
    0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
  };

  vtkm::cont::DataSetBuilderUniform builder;
  vtkm::cont::DataSet dataSet = builder.Create(vtkm::Id3(8, 8, 1));

  vtkm::cont::DataSetFieldAdd dataSetFieldAdd;
  dataSetFieldAdd.AddPointField(dataSet, "color", pixels);

  return dataSet;
}

void TestImageConnectivity()
{
  vtkm::cont::DataSet dataSet = MakeTestDataSet();

  vtkm::filter::ImageConnectivity connectivity;
  connectivity.SetActiveField("color");

  const vtkm::cont::DataSet outputData = connectivity.Execute(dataSet);

  auto temp = outputData.GetField("component").GetData();
  vtkm::cont::ArrayHandle<vtkm::Id> resultArrayHandle;
  temp.CopyTo(resultArrayHandle);

  std::vector<vtkm::Id> componentExpected = { 0, 1, 1, 1, 0, 1, 1, 2, 0, 0, 0, 1, 0, 1, 1, 2,
                                              0, 1, 1, 0, 0, 1, 1, 2, 0, 1, 0, 0, 0, 1, 1, 2,
                                              0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
                                              0, 1, 0, 1, 1, 1, 3, 3, 0, 1, 1, 1, 1, 1, 3, 3 };

  for (vtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(
      test_equal(resultArrayHandle.GetPortalConstControl().Get(i), componentExpected[size_t(i)]),
      "Wrong result for ImageConnectivity");
  }
}
}

int UnitTestImageConnectivityFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestImageConnectivity, argc, argv);
}
