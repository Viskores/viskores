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

#include <vtkm/filter/geometry_refinement/SplitSharpEdges.h>
#include <vtkm/filter/vector_analysis/SurfaceNormals.h>

#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{
void TestSplitSharpEdges()
{
  std::cout << "Generate Image for SplitSharpEdges filter" << std::endl;

  auto pathname =
    vtkm::cont::testing::Testing::DataPath("unstructured/SplitSharpEdgesTestDataSet.vtk");
  vtkm::io::VTKDataSetReader reader(pathname);
  auto dataSet = reader.ReadDataSet();

  vtkm::filter::geometry_refinement::SplitSharpEdges splitSharpEdges;
  splitSharpEdges.SetFeatureAngle(89.0);
  splitSharpEdges.SetActiveField("Normals", vtkm::cont::Field::Association::Cells);

  auto result = splitSharpEdges.Execute(dataSet);
  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.ColorTable = vtkm::cont::ColorTable::Preset::Inferno;
  testOptions.EnableAnnotations = false;
  vtkm::rendering::testing::RenderTest(
    result, "pointvar", "filter/split-sharp-edges.png", testOptions);
}
} // namespace

int RenderTestSplitSharpEdges(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSplitSharpEdges, argc, argv);
}
