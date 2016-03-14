//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/filter/CellAverage.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

namespace {

void TestCellAverageRegular3D()
{
  std::cout << "Testing CellAverage Filter on 3D strucutred data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::filter::FieldResult result;
  vtkm::filter::CellAverage cavg;
  cavg.SetOutputFieldName("avgvals");

  result = cavg.Execute( dataSet, dataSet.GetField("pointvar"));

  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  const bool valid = result.FieldAs(resultArrayHandle);

  if(valid)
  {
    vtkm::Float32 expected[4] = { 60.1875f, 70.2125f, 120.3375f, 130.3625f };
    for (int i = 0; i < 4; ++i)
    {
      VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i),
          expected[i]), "Wrong result for CellAverage worklet on 3D regular data");
    }
  }
}

void TestCellAverageRegular2D()
{
  std::cout << "Testing CellAverage Filter on 2D strucutred data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::filter::FieldResult result;
  vtkm::filter::CellAverage cavg;
  cavg.SetOutputFieldName("avgvals");

  result = cavg.Execute( dataSet, dataSet.GetField("pointvar"));

  vtkm::cont::Field resultField = result.GetField();
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  resultField.GetData().CopyTo(resultArrayHandle);

  if(result.IsValid())
  {
    vtkm::Float32 expected[2] = { 30.1f, 40.1f };
    for (int i = 0; i < 2; ++i)
    {
      VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i),
          expected[i]), "Wrong result for CellAverage worklet on 2D regular data");
    }
  }
}

void TestCellAverageExplicit()
{
  std::cout << "Testing CellAverage Filter on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  vtkm::filter::FieldResult result;
  vtkm::filter::CellAverage cavg;
  cavg.SetOutputFieldName("avgvals");

  result = cavg.Execute( dataSet, dataSet.GetField("pointvar"));

  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  const bool valid = result.FieldAs(resultArrayHandle);

  if(valid)
  {
    vtkm::Float32 expected[2] = { 20.1333f, 35.2f };
    for (int i = 0; i < 2; ++i)
    {
      VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i),
          expected[i]), "Wrong result for CellAverage worklet on 3D regular data");
    }
  }
}


void TestCellAverage()
{
  TestCellAverageRegular2D();
  TestCellAverageRegular3D();
  TestCellAverageExplicit();
}

}

int UnitTestCellAverageFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestCellAverage);
}
