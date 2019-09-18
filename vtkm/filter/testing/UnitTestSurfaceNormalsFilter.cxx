//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/SurfaceNormals.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void VerifyCellNormalValues(const vtkm::cont::DataSet& ds)
{
  vtkm::cont::ArrayHandle<vtkm::Vec3f> normals;
  ds.GetCellField("Normals").GetData().CopyTo(normals);

  vtkm::Vec3f expected[8] = { { -0.707f, -0.500f, 0.500f }, { -0.707f, -0.500f, 0.500f },
                              { 0.707f, 0.500f, -0.500f },  { 0.000f, -0.707f, -0.707f },
                              { 0.000f, -0.707f, -0.707f }, { 0.000f, 0.707f, 0.707f },
                              { -0.707f, 0.500f, -0.500f }, { 0.707f, -0.500f, 0.500f } };

  auto portal = normals.GetPortalConstControl();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == 8, "incorrect normals array length");
  for (vtkm::Id i = 0; i < 8; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(i), expected[i], 0.001),
                     "result does not match expected value");
  }
}

void VerifyPointNormalValues(const vtkm::cont::DataSet& ds)
{
  vtkm::cont::ArrayHandle<vtkm::Vec3f> normals;
  ds.GetPointField("Normals").GetData().CopyTo(normals);

  vtkm::Vec3f expected[8] = { { -0.8165f, -0.4082f, -0.4082f }, { -0.2357f, -0.9714f, 0.0286f },
                              { 0.0000f, -0.1691f, 0.9856f },   { -0.8660f, 0.0846f, 0.4928f },
                              { 0.0000f, -0.1691f, -0.9856f },  { 0.0000f, 0.9856f, -0.1691f },
                              { 0.8165f, 0.4082f, 0.4082f },    { 0.8165f, -0.4082f, -0.4082f } };

  auto portal = normals.GetPortalConstControl();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == 8, "incorrect normals array length");
  for (vtkm::Id i = 0; i < 8; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(i), expected[i], 0.001),
                     "result does not match expected value");
  }
}

void TestSurfaceNormals()
{
  vtkm::cont::DataSet ds = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetPolygonal();

  vtkm::filter::SurfaceNormals filter;
  vtkm::cont::DataSet result;

  std::cout << "testing default output (generate only point normals):\n";
  result = filter.Execute(ds);
  VTKM_TEST_ASSERT(result.HasPointField("Normals"), "Point normals missing.");

  std::cout << "generate only cell normals:\n";
  filter.SetGenerateCellNormals(true);
  filter.SetGeneratePointNormals(false);
  result = filter.Execute(ds);
  VTKM_TEST_ASSERT(result.HasCellField("Normals"), "Cell normals missing.");

  std::cout << "generate both cell and point normals:\n";
  filter.SetGeneratePointNormals(true);
  filter.SetAutoOrientNormals(true);
  result = filter.Execute(ds);
  VTKM_TEST_ASSERT(result.HasPointField("Normals"), "Point normals missing.");
  VTKM_TEST_ASSERT(result.HasCellField("Normals"), "Cell normals missing.");

  std::cout << "test result values:\n";
  VerifyPointNormalValues(result);
  VerifyCellNormalValues(result);
}

} // anonymous namespace


int UnitTestSurfaceNormalsFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSurfaceNormals, argc, argv);
}
