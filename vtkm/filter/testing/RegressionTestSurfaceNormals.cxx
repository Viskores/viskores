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
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/VTKDataSetReader.h>

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

  // NOTE: This dataset stores a shape value of 7 for polygons.  The
  // VTKDataSetReader currently converts all polygons with 4 verticies to
  // quads (shape 9).
  auto pathname =
    vtkm::cont::testing::Testing::DataPath("unstructured/SurfaceNormalsTestDataSet.vtk");
  vtkm::io::VTKDataSetReader reader(pathname);
  auto dataSet = reader.ReadDataSet();

  vtkm::filter::SurfaceNormals surfaceNormals;
  surfaceNormals.SetGeneratePointNormals(true);
  surfaceNormals.SetAutoOrientNormals(true);

  auto result = surfaceNormals.Execute(dataSet);
  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result, "pointvar", colorTable, "filter/surface-normals.png", false);
}
} // namespace

int RegressionTestSurfaceNormals(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSurfaceNormals, argc, argv);
}
