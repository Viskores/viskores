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

#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/source/Tangle.h>

#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{
void TestContourFilterWedge()
{
  std::cout << "Generate Image for Contour filter on an unstructured grid" << std::endl;

  auto pathname = vtkm::cont::testing::Testing::DataPath("unstructured/wedge_cells.vtk");
  vtkm::io::VTKDataSetReader reader(pathname);
  vtkm::cont::DataSet dataSet = reader.ReadDataSet();

  vtkm::filter::contour::Contour contour;
  contour.SetIsoValues({ -1, 0, 1 });
  contour.SetActiveField("gyroid");
  contour.SetFieldsToPass({ "gyroid", "cellvar" });
  contour.SetMergeDuplicatePoints(true);
  auto result = contour.Execute(dataSet);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  vtkm::rendering::testing::RenderTest(result, "gyroid", "filter/contour-wedge.png", testOptions);
}

void TestContourFilterUniform()
{
  std::cout << "Generate Image for Contour filter on a uniform grid" << std::endl;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet inputData = maker.Make3DUniformDataSet0();
  std::string fieldName = "pointvar";
  VTKM_TEST_ASSERT(inputData.HasField(fieldName));

  vtkm::filter::contour::Contour contour;
  contour.SetGenerateNormals(false);
  contour.SetMergeDuplicatePoints(true);
  contour.SetIsoValues({ 50, 100, 150 });
  contour.SetActiveField(fieldName);
  contour.SetFieldsToPass(fieldName);
  vtkm::cont::DataSet result = contour.Execute(inputData);

  result.PrintSummary(std::cout);

  //Y axis Flying Edge algorithm has subtle differences at a couple of boundaries
  vtkm::rendering::testing::RenderTestOptions testOptions;
  vtkm::rendering::testing::RenderTest(
    result, "pointvar", "filter/contour-uniform.png", testOptions);
}

void TestContourFilterTangle()
{
  std::cout << "Generate Image for Contour filter on a uniform tangle grid" << std::endl;

  vtkm::Id3 dims(4, 4, 4);
  vtkm::source::Tangle tangle(dims);
  vtkm::cont::DataSet dataSet = tangle.Execute();

  vtkm::filter::contour::Contour contour;
  contour.SetGenerateNormals(true);
  contour.SetIsoValue(0, 1);
  contour.SetActiveField("tangle");
  contour.SetFieldsToPass("tangle");
  auto result = contour.Execute(dataSet);

  result.PrintSummary(std::cout);

  //Y axis Flying Edge algorithm has subtle differences at a couple of boundaries
  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.Colors = { { 0.20f, 0.80f, 0.20f } };
  testOptions.EnableAnnotations = false;
  vtkm::rendering::testing::RenderTest(result, "tangle", "filter/contour-tangle.png", testOptions);
}

void TestContourFilter()
{
  TestContourFilterUniform();
  TestContourFilterTangle();
  TestContourFilterWedge();
}
} // namespace

int RenderTestContourFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourFilter, argc, argv);
}
