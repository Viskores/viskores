//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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

void TestFacetedSurfaceNormals()
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  vtkm::cont::DataSet ds = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetPolygonal();

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> normals;

  vtkm::worklet::FacetedSurfaceNormals worklet;
  worklet.Run(ds.GetCellSet(), ds.GetCoordinateSystem().GetData(), normals, DeviceAdapter());

  vtkm::Vec<vtkm::FloatDefault, 3> expNormals[8] = {
    { -0.707f, -0.500f, 0.500f }, { -0.707f, -0.500f, 0.500f }, { 0.707f, 0.500f, -0.500f },
    { 0.000f, -0.707f, -0.707f }, { 0.000f, -0.707f, -0.707f }, { 0.000f, 0.707f, 0.707f },
    { -0.707f, 0.500f, -0.500f }, { 0.707f, -0.500f, 0.500f }
  };

  auto portal = normals.GetPortalConstControl();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == 8, "incorrect normals array length");
  for (vtkm::Id i = 0; i < 8; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(i), expNormals[i], 0.001),
                     "result does not match expected value");
  }
}

} // anonymous namespace

int UnitTestSurfaceNormals(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestFacetedSurfaceNormals);
}
