//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/SurfaceNormals.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{
void TestSurfaceNormals()
{
  std::cout << "Generate Image for SurfaceNormals filter" << std::endl;

  vtkm::cont::ColorTable colorTable("inferno");
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::cont::testing::MakeTestDataSet makeTestData;
  auto dataSet = makeTestData.Make3DExplicitDataSetPolygonal();

  vtkm::filter::SurfaceNormals surfaceNormals;
  surfaceNormals.SetGeneratePointNormals(true);
  surfaceNormals.SetAutoOrientNormals(true);

  auto result = surfaceNormals.Execute(dataSet);
  result.PrintSummary(std::cout);

  C canvas(512, 512);
  M mapper;
  vtkm::rendering::Scene scene;
  auto view = vtkm::rendering::testing::GetViewPtr<M, C, V3>(
    result, "pointvar", canvas, mapper, scene, colorTable, static_cast<vtkm::FloatDefault>(0.05));

  VTKM_TEST_ASSERT(vtkm::rendering::testing::test_equal_images(view, "surface-normals.png"));
}
} // namespace

int RegressionTestSurfaceNormals(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSurfaceNormals, argc, argv);
}
