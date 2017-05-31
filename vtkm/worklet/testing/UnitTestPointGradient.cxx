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

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/Gradient.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename DeviceAdapter>
void TestPointGradientUniform2D()
{
  std::cout << "Testing PointGradient Worklet on 2D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("pointvar").GetData().CopyTo(fieldArray);

  vtkm::worklet::PointGradient gradient;
  auto result =
    gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), fieldArray, DeviceAdapter());

  vtkm::Vec<vtkm::Float32, 3> expected[2] = { { 10, 30, 0 }, { 10, 30, 0 } };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 2D uniform data");
  }
}

template <typename DeviceAdapter>
void TestPointGradientUniform3D()
{
  std::cout << "Testing PointGradient Worklet on 3D strucutred data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("pointvar").GetData().CopyTo(fieldArray);

  vtkm::worklet::PointGradient gradient;
  auto result =
    gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), fieldArray, DeviceAdapter());

  vtkm::Vec<vtkm::Float32, 3> expected[4] = {
    { 10.0, 30, 60.1 }, { 10.0, 30.1, 60.1 }, { 10.0, 30.1, 60.2 }, { 10.1, 30, 60.2 },
  };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 3D uniform data");
  }
}

template <typename DeviceAdapter>
void TestPointGradientUniform3DWithVectorField()
{
  std::cout << "Testing PointGradient Worklet with a vector field on 3D strucutred data"
            << std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //Verify that we can compute the gradient of a 3 component vector
  const int nVerts = 18;
  vtkm::Float64 vars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };
  std::vector<vtkm::Vec<vtkm::Float64, 3>> vec(18);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = vtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>> input = vtkm::cont::make_ArrayHandle(vec);

  vtkm::worklet::PointGradient gradient;
  auto result =
    gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input, DeviceAdapter());


  vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3> expected[4] = {
    { { 10.0, 10.0, 10.0 }, { 30.0, 30.0, 30.0 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.2, 60.2, 60.2 } },
    { { 10.1, 10.1, 10.1 }, { 30.0, 30.0, 30.0 }, { 60.2, 60.2, 60.2 } }
  };
  for (int i = 0; i < 4; ++i)
  {
    vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3> e = expected[i];
    vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3> r = result.GetPortalConstControl().Get(i);

    VTKM_TEST_ASSERT(test_equal(e[0], r[0]),
                     "Wrong result for vec field PointGradient worklet on 3D uniform data");
    VTKM_TEST_ASSERT(test_equal(e[1], r[1]),
                     "Wrong result for vec field PointGradient worklet on 3D uniform data");
    VTKM_TEST_ASSERT(test_equal(e[2], r[2]),
                     "Wrong result for vec field PointGradient worklet on 3D uniform data");
  }
}

template <typename DeviceAdapter>
void TestPointGradientExplicit()
{
  std::cout << "Testing PointGradient Worklet on Explicit data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("pointvar").GetData().CopyTo(fieldArray);

  vtkm::worklet::PointGradient gradient;
  auto result =
    gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), fieldArray, DeviceAdapter());

  vtkm::Vec<vtkm::Float32, 3> expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, 0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for PointGradient worklet on 3D explicit data");
  }
}

void TestPointGradient()
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  TestPointGradientUniform2D<DeviceAdapter>();
  TestPointGradientUniform3D<DeviceAdapter>();
  TestPointGradientUniform3DWithVectorField<DeviceAdapter>();
  TestPointGradientExplicit<DeviceAdapter>();
}
}

int UnitTestPointGradient(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestPointGradient);
}
