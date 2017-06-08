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
#include <vtkm/filter/SurfaceNormals.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestSurfaceNormals()
{
  vtkm::cont::DataSet ds = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetPolygonal();

  vtkm::filter::SurfaceNormals filter;
  auto result = filter.Execute(ds);

  VTKM_TEST_ASSERT(result.GetField().GetName() == "normals", "Field was given the wrong name.");
  VTKM_TEST_ASSERT(result.GetField().GetAssociation() == vtkm::cont::Field::ASSOC_CELL_SET,
                   "Field was given the wrong association.");

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> normals;
  result.FieldAs(normals);

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


int UnitTestSurfaceNormalsFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestSurfaceNormals);
}
