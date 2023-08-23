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

vtkm::cont::DataSet MakeTestDatasetStructured2D()
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

vtkm::cont::DataSet MakeTestDatasetStructured3D()
{
  static constexpr vtkm::Id xdim = 3, ydim = 3, zdim = 3;
  static const vtkm::Id3 dim(xdim, ydim, zdim);
  static constexpr vtkm::Id numVerts = xdim * ydim * zdim;
  vtkm::Float32 scalars[numVerts];
  for (vtkm::Id i = 0; i < numVerts; i++)
  {
    scalars[i] = static_cast<vtkm::Float32>(i * 0.1);
  }
  scalars[13] = 0.0f;
  vtkm::cont::DataSet ds;
  ds = vtkm::cont::DataSetBuilderUniform::Create(
    dim, vtkm::Vec3f(-1.0f, -1.0f, -1.0f), vtkm::Vec3f(1, 1, 1));
  ds.AddPointField("scalars", scalars, numVerts);
  return ds;
}

void TestClipStructuredSphere(vtkm::Float64 offset)
{
  std::cout << "Testing ClipWithImplicitFunction Filter on Structured data with Sphere function"
            << std::endl;

  vtkm::cont::DataSet ds = MakeTestDatasetStructured2D();

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

  VTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 12,
                   "Wrong number of points in the output dataset");

  vtkm::Float32 expected[12] = { 1, 1, 1, 1, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 12; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                     "Wrong result for ClipWithImplicitFunction fliter on sturctured quads data");
  }
}

void TestClipStructuredInvertedSphere()
{
  std::cout
    << "Testing ClipWithImplicitFunctionInverted Filter on Structured data with Sphere function"
    << std::endl;

  vtkm::cont::DataSet ds = MakeTestDatasetStructured2D();

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

  VTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 5,
                   "Wrong number of points in the output dataset");

  vtkm::Float32 expected[5] = { 0, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 5; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                     "Wrong result for ClipWithImplicitFunction fliter on sturctured quads data");
  }
}

void TestClipStructuredInvertedMultiPlane()
{
  std::cout << "Testing TestClipStructured Filter on Structured data with MultiPlane function"
            << std::endl;
  vtkm::cont::DataSet ds = MakeTestDatasetStructured3D();
  vtkm::filter::contour::ClipWithImplicitFunction clip;
  vtkm::MultiPlane<3> TriplePlane;
  //set xy plane
  TriplePlane.AddPlane(vtkm::Vec3f{ 1.0f, 1.0f, 0.0f }, vtkm::Vec3f{ 0.0f, 0.0f, 1.0f });
  //set yz plane
  TriplePlane.AddPlane(vtkm::Vec3f{ 0.0f, 1.0f, 1.0f }, vtkm::Vec3f{ 1.0f, 0.0f, 0.0f });
  //set xz plane
  TriplePlane.AddPlane(vtkm::Vec3f{ 1.0f, 0.0f, 1.0f }, vtkm::Vec3f{ 0.0f, 1.0f, 0.0f });
  clip.SetInvertClip(true);
  clip.SetImplicitFunction(TriplePlane);
  clip.SetFieldsToPass("scalars");
  auto outputData = clip.Execute(ds);

  VTKM_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                   "Wrong number of fields in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfCells() == 1,
                   "Wrong number of cells in the output dataset");
  vtkm::cont::UnknownArrayHandle temp = outputData.GetField("scalars").GetData();
  vtkm::cont::ArrayHandle<vtkm::Float32> resultArrayHandle;
  temp.AsArrayHandle(resultArrayHandle);
  vtkm::Float32 expected[4] = { 0.0f, 0.1f, 0.3f, 0.9f };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                     "Wrong result for ClipWithImplicitFunction fliter on sturctured data in "
                     "TestClipStructuredInvertedMultiPlane");
  }
}

void TestClip()
{
  //todo: add more clip tests
  TestClipStructuredSphere(-0.2);
  TestClipStructuredSphere(0.0);
  TestClipStructuredSphere(0.2);
  TestClipStructuredInvertedSphere();
  TestClipStructuredInvertedMultiPlane();
}

} // anonymous namespace

int UnitTestClipWithImplicitFunctionFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestClip, argc, argv);
}
