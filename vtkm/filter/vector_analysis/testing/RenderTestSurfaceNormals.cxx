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
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/filter/vector_analysis/SurfaceNormals.h>

#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{
void TestSurfaceNormals()
{
  std::cout << "Generate Image for SurfaceNormals filter" << std::endl;

  // NOTE: This dataset stores a shape value of 7 for polygons.  The
  // VTKDataSetReader currently converts all polygons with 4 verticies to
  // quads (shape 9).
  auto pathname =
    vtkm::cont::testing::Testing::DataPath("unstructured/SurfaceNormalsTestDataSet.vtk");
  vtkm::io::VTKDataSetReader reader(pathname);
  auto dataSet = reader.ReadDataSet();

  vtkm::filter::vector_analysis::SurfaceNormals surfaceNormals;
  surfaceNormals.SetGeneratePointNormals(true);
  surfaceNormals.SetAutoOrientNormals(true);

  auto result = surfaceNormals.Execute(dataSet);
  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;
  testOptions.EnableAnnotations = false;
  vtkm::rendering::testing::RenderTest(
    result, "pointvar", "filter/surface-normals.png", testOptions);
}
} // namespace

int RenderTestSurfaceNormals(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSurfaceNormals, argc, argv);
}
