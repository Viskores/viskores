//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/contour/ClipWithImplicitFunction.h>

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

using Coord3D = vtkm::Vec3f;

vtkm::cont::DataSet MakeTestDatasetStructured()
{
  static constexpr vtkm::Id xdim = 3, ydim = 3;
  static const vtkm::Id2 dim(xdim, ydim);
  static constexpr vtkm::Id numVerts = xdim * ydim;

  vtkm::Float32 scalars[numVerts];
  for (float& scalar : scalars)
  {
    scalar = 1.0f;
  }
  scalars[4] = 0.0f;

  vtkm::cont::DataSet ds;
  ds = vtkm::cont::DataSetBuilderUniform::Create(dim);

  ds.AddPointField("scalars", scalars, numVerts);

  return ds;
}

void TestClipStructured(vtkm::Float64 offset)
{
  std::cout << "Testing ClipWithImplicitFunction Filter on Structured data" << std::endl;

  vtkm::cont::DataSet ds = MakeTestDatasetStructured();

  vtkm::Vec3f center(1, 1, 0);

  // the `expected` results are based on radius = 0.5 and offset = 0.
  // for a given offset, compute the radius that would produce the same results
  auto radius = static_cast<vtkm::FloatDefault>(vtkm::Sqrt(0.25 - offset));

  std::cout << "offset = " << offset << ", radius = " << radius << std::endl;

  vtkm::filter::contour::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(vtkm::Sphere(center, radius));
  clip.SetOffset(offset);
  clip.SetFieldsToPass("scalars");

  vtkm::cont::DataSet outputData = clip.Execute(ds);

  VTKM_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                   "Wrong number of fields in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfCells() == 8,
                   "Wrong number of cells in the output dataset");

  vtkm::cont::UnknownArrayHandle temp = outputData.GetField("scalars").GetData();
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  temp.AsArrayHandle(resultArrayHandle);

  VTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 13,
                   "Wrong number of points in the output dataset");

  vtkm::Float32 expected[13] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 13; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                     "Wrong result for ClipWithImplicitFunction fliter on sturctured quads data");
  }
}

void TestClipStructuredInverted()
{
  std::cout << "Testing ClipWithImplicitFunctionInverted Filter on Structured data" << std::endl;

  vtkm::cont::DataSet ds = MakeTestDatasetStructured();

  vtkm::Vec3f center(1, 1, 0);
  vtkm::FloatDefault radius(0.5);

  vtkm::filter::contour::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(vtkm::Sphere(center, radius));
  bool invert = true;
  clip.SetInvertClip(invert);
  clip.SetFieldsToPass("scalars");
  auto outputData = clip.Execute(ds);

  VTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                   "Wrong number of fields in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfCells() == 4,
                   "Wrong number of cells in the output dataset");

  vtkm::cont::UnknownArrayHandle temp = outputData.GetField("scalars").GetData();
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  temp.AsArrayHandle(resultArrayHandle);

  VTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 13,
                   "Wrong number of points in the output dataset");

  vtkm::Float32 expected[13] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 13; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                     "Wrong result for ClipWithImplicitFunction fliter on sturctured quads data");
  }
}

void TestClip()
{
  //todo: add more clip tests
  TestClipStructured(-0.2);
  TestClipStructured(0.0);
  TestClipStructured(0.2);
  TestClipStructuredInverted();
}

} // anonymous namespace

int UnitTestClipWithImplicitFunctionFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestClip, argc, argv);
}
