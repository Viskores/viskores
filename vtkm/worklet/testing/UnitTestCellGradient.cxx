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

#include <vtkm/worklet/Gradient.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

namespace {


void TestCellGradientUniform2D()
{
  std::cout << "Testing CellGradient Worklet on 2D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > result;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellGradient> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(),
                    dataSet.GetCoordinateSystem(),
                    dataSet.GetField("pointvar"),
                    result);

  vtkm::Vec<vtkm::Float32,3> expected[2] = { {10,30,0}, {10,30,0} };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(
          test_equal(result.GetPortalConstControl().Get(i), expected[i]),
          "Wrong result for CellGradient worklet on 2D uniform data");
  }
}


void TestCellGradientUniform3D()
{
  std::cout << "Testing CellGradient Worklet on 3D strucutred data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float64,3> > result;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellGradient> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(),
                    dataSet.GetCoordinateSystem(),
                    dataSet.GetField("pointvar"),
                    result);

  vtkm::Vec<vtkm::Float64,3> expected[4] = { {10.025,30.075,60.125},
                                             {10.025,30.075,60.125},
                                             {10.025,30.075,60.175},
                                             {10.025,30.075,60.175},
                                           };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(
          test_equal(result.GetPortalConstControl().Get(i), expected[i]),
          "Wrong result for CellGradient worklet on 3D uniform data");
  }
}

void TestCellGradientExplicit()
{
  std::cout << "Testing CellGradient Worklet on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > result;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellGradient> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(),
                    dataSet.GetCoordinateSystem(),
                    dataSet.GetField("pointvar"),
                    result);

  vtkm::Vec<vtkm::Float32,3> expected[2] = { {10.f,10.1f,0.0f}, {10.f,10.1f,-0.0f} };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(
          test_equal(result.GetPortalConstControl().Get(i), expected[i]),
          "Wrong result for CellGradient worklet on 3D explicit data");
  }
}


void TestCellGradient()
{
  TestCellGradientUniform2D();
  TestCellGradientUniform3D();
  TestCellGradientExplicit();
}

}

int UnitTestCellGradient(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestCellGradient);
}
