//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CellAverage.h>

namespace
{

void TestCellAverageRegular3D()
{
  std::cout << "Testing CellAverage Filter on 3D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::filter::CellAverage cellAverage;
  cellAverage.SetOutputFieldName("avgvals");
  cellAverage.SetActiveField("pointvar");
  vtkm::cont::DataSet result = cellAverage.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasField("avgvals", vtkm::cont::Field::Association::CELL_SET) == true,
                   "Result field not present.");

  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  result.GetField("avgvals", vtkm::cont::Field::Association::CELL_SET)
    .GetData()
    .CopyTo(resultArrayHandle);
  {
    vtkm::Float32 expected[4] = { 60.1875f, 70.2125f, 120.3375f, 130.3625f };
    for (vtkm::Id i = 0; i < 4; ++i)
    {
      VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                       "Wrong result for CellAverage worklet on 3D regular data");
    }
  }

  std::cout << "Run again for point coordinates" << std::endl;
  cellAverage.SetOutputFieldName("avgpos");
  cellAverage.SetUseCoordinateSystemAsField(true);
  result = cellAverage.Execute(dataSet);

  VTKM_TEST_ASSERT(result.HasField("avgpos", vtkm::cont::Field::Association::CELL_SET),
                   "Result field not present.");

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> resultPointArray;
  vtkm::cont::Field resultPointField =
    result.GetField("avgpos", vtkm::cont::Field::Association::CELL_SET);
  resultPointField.GetData().CopyTo(resultPointArray);
  {
    vtkm::FloatDefault expected[4][3] = {
      { 0.5f, 0.5f, 0.5f }, { 1.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 1.5f }, { 1.5f, 0.5f, 1.5f }
    };
    for (vtkm::Id i = 0; i < 4; ++i)
    {
      vtkm::Vec<vtkm::FloatDefault, 3> expectedVec(expected[i][0], expected[i][1], expected[i][2]);
      vtkm::Vec<vtkm::FloatDefault, 3> computedVec(resultPointArray.GetPortalConstControl().Get(i));
      VTKM_TEST_ASSERT(test_equal(computedVec, expectedVec),
                       "Wrong result for CellAverage worklet on 3D regular data");
    }
  }
}

void TestCellAverageRegular2D()
{
  std::cout << "Testing CellAverage Filter on 2D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::filter::CellAverage cellAverage;
  cellAverage.SetActiveField("pointvar");

  vtkm::cont::DataSet result = cellAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VTKM_TEST_ASSERT(result.HasField("pointvar", vtkm::cont::Field::Association::CELL_SET),
                   "Field missing.");

  vtkm::cont::Field resultField =
    result.GetField("pointvar", vtkm::cont::Field::Association::CELL_SET);
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  resultField.GetData().CopyTo(resultArrayHandle);
  vtkm::Float32 expected[2] = { 30.1f, 40.1f };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 2D regular data");
  }
}

void TestCellAverageExplicit()
{
  std::cout << "Testing CellAverage Filter on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  vtkm::filter::CellAverage cellAverage;
  cellAverage.SetActiveField("pointvar");

  vtkm::cont::DataSet result = cellAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VTKM_TEST_ASSERT(result.HasField("pointvar", vtkm::cont::Field::Association::CELL_SET),
                   "Field missing.");

  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  vtkm::cont::Field resultField =
    result.GetField("pointvar", vtkm::cont::Field::Association::CELL_SET);
  resultField.GetData().CopyTo(resultArrayHandle);
  vtkm::Float32 expected[2] = { 20.1333f, 35.2f };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 3D regular data");
  }
}

void TestCellAverage()
{
  TestCellAverageRegular2D();
  TestCellAverageRegular3D();
  TestCellAverageExplicit();
}
}

int UnitTestCellAverageFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestCellAverage);
}
