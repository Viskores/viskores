//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/worklet/SurfaceNormals.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

namespace
{

using NormalsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>;
using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;

void TestFacetedSurfaceNormals(const vtkm::cont::DataSet& dataset, NormalsArrayHandle& normals)
{
  std::cout << "Testing FacetedSurfaceNormals:\n";

  vtkm::worklet::FacetedSurfaceNormals faceted;
  faceted.Run(
    dataset.GetCellSet(), dataset.GetCoordinateSystem().GetData(), normals, DeviceAdapter());

  vtkm::Vec<vtkm::FloatDefault, 3> expected[8] = {
    { -0.707f, -0.500f, 0.500f }, { -0.707f, -0.500f, 0.500f }, { 0.707f, 0.500f, -0.500f },
    { 0.000f, -0.707f, -0.707f }, { 0.000f, -0.707f, -0.707f }, { 0.000f, 0.707f, 0.707f },
    { -0.707f, 0.500f, -0.500f }, { 0.707f, -0.500f, 0.500f }
  };
  auto portal = normals.GetPortalConstControl();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == 8, "incorrect faceNormals array length");
  for (vtkm::Id i = 0; i < 8; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(i), expected[i], 0.001),
                     "result does not match expected value");
  }
}

void TestSmoothSurfaceNormals(const vtkm::cont::DataSet& dataset,
                              const NormalsArrayHandle& faceNormals)
{
  std::cout << "Testing SmoothSurfaceNormals:\n";

  NormalsArrayHandle pointNormals;
  vtkm::worklet::SmoothSurfaceNormals smooth;
  smooth.Run(dataset.GetCellSet(), faceNormals, pointNormals, DeviceAdapter());

  vtkm::Vec<vtkm::FloatDefault, 3> expected[8] = {
    { -0.8165f, -0.4082f, -0.4082f }, { -0.2357f, -0.9714f, 0.0286f },
    { 0.0000f, -0.1691f, 0.9856f },   { -0.8660f, 0.0846f, 0.4928f },
    { 0.0000f, -0.1691f, -0.9856f },  { 0.0000f, 0.9856f, -0.1691f },
    { 0.8165f, 0.4082f, 0.4082f },    { 0.8165f, -0.4082f, -0.4082f }
  };
  auto portal = pointNormals.GetPortalConstControl();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == 8, "incorrect pointNormals array length");
  for (vtkm::Id i = 0; i < 8; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(i), expected[i], 0.001),
                     "result does not match expected value");
  }
}

void TestSurfaceNormals()
{
  vtkm::cont::DataSet dataset =
    vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetPolygonal();
  NormalsArrayHandle faceNormals;

  TestFacetedSurfaceNormals(dataset, faceNormals);
  TestSmoothSurfaceNormals(dataset, faceNormals);
}

} // anonymous namespace

int UnitTestSurfaceNormals(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestSurfaceNormals);
}
