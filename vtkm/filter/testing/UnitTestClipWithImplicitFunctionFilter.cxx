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

#include <vtkm/filter/ClipWithImplicitFunction.h>

#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

using Coord3D = vtkm::Vec<vtkm::FloatDefault, 3>;

vtkm::cont::DataSet MakeTestDatasetStructured()
{
  static constexpr vtkm::Id xdim = 3, ydim = 3;
  static const vtkm::Id2 dim(xdim, ydim);
  static constexpr vtkm::Id numVerts = xdim * ydim;

  vtkm::Float32 scalars[numVerts];
  for (vtkm::Id i = 0; i < numVerts; ++i)
  {
    scalars[i] = 1.0f;
  }
  scalars[4] = 0.0f;

  vtkm::cont::DataSet ds;
  vtkm::cont::DataSetBuilderUniform builder;
  ds = builder.Create(dim);

  vtkm::cont::DataSetFieldAdd fieldAdder;
  fieldAdder.AddPointField(ds, "scalars", scalars, numVerts);

  return ds;
}

void TestClipStructured()
{
  std::cout << "Testing ClipWithImplicitFunction Filter on Structured data" << std::endl;

  vtkm::cont::DataSet ds = MakeTestDatasetStructured();

  vtkm::Vec<vtkm::FloatDefault, 3> center(1, 1, 0);
  vtkm::FloatDefault radius(0.5);

  vtkm::filter::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(vtkm::cont::make_ImplicitFunctionHandle(vtkm::Sphere(center, radius)));
  clip.SetFieldsToPass("scalars");

  vtkm::cont::DataSet outputData = clip.Execute(ds);

  VTKM_TEST_ASSERT(outputData.GetNumberOfCellSets() == 1,
                   "Wrong number of cellsets in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 1,
                   "Wrong number of fields in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetCellSet().GetNumberOfCells() == 12,
                   "Wrong number of cells in the output dataset");

  vtkm::cont::DynamicArrayHandle temp = outputData.GetField("scalars").GetData();
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  temp.CopyTo(resultArrayHandle);

  VTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 13,
                   "Wrong number of points in the output dataset");

  vtkm::Float32 expected[13] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 13; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for ClipWithImplicitFunction fliter on sturctured quads data");
  }
}

void TestClipStructuredInverted()
{
  std::cout << "Testing ClipWithImplicitFunctionInverted Filter on Structured data" << std::endl;

  vtkm::cont::DataSet ds = MakeTestDatasetStructured();

  vtkm::Vec<vtkm::FloatDefault, 3> center(1, 1, 0);
  vtkm::FloatDefault radius(0.5);

  vtkm::filter::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(vtkm::cont::make_ImplicitFunctionHandle(vtkm::Sphere(center, radius)));
  bool invert = true;
  clip.SetInvertClip(invert);
  clip.SetFieldsToPass("scalars");
  auto outputData = clip.Execute(ds);
  VTKM_TEST_ASSERT(outputData.GetNumberOfCellSets() == 1,
                   "Wrong number of cellsets in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 1,
                   "Wrong number of fields in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetCellSet().GetNumberOfCells() == 4,
                   "Wrong number of cells in the output dataset");

  vtkm::cont::DynamicArrayHandle temp = outputData.GetField("scalars").GetData();
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  temp.CopyTo(resultArrayHandle);

  VTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 13,
                   "Wrong number of points in the output dataset");

  vtkm::Float32 expected[13] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 13; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for ClipWithImplicitFunction fliter on sturctured quads data");
  }
}

void TestClip()
{
  //todo: add more clip tests
  TestClipStructured();
  TestClipStructuredInverted();
}

} // anonymous namespace

int UnitTestClipWithImplicitFunctionFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestClip);
}
