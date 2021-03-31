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

#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/Contour.h>
#include <vtkm/source/Tangle.h>

#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{
void TestContourFilterWedge()
{
  std::cout << "Generate Image for Contour filter on an unstructured grid" << std::endl;

  vtkm::cont::ColorTable colorTable(
    { 0, 1 }, { 0.20f, 0.80f, 0.20f }, { 0.20f, 0.80f, 0.201f }, vtkm::ColorSpace::RGB);
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  auto pathname = vtkm::cont::testing::Testing::DataPath("unstructured/wedge_cells.vtk");
  vtkm::io::VTKDataSetReader reader(pathname);
  vtkm::cont::DataSet dataSet = reader.ReadDataSet();

  vtkm::filter::Contour contour;
  contour.SetIsoValue(0, 1);
  contour.SetActiveField("gyroid");
  contour.SetFieldsToPass({ "gyroid", "cellvar" });
  contour.SetMergeDuplicatePoints(false);
  auto result = contour.Execute(dataSet);

  result.PrintSummary(std::cout);

  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result,
    "gyroid",
    colorTable,
    "filter/contour-wedge.png",
    false,
    static_cast<vtkm::FloatDefault>(0.08));
}

void TestContourFilterUniform()
{
  std::cout << "Generate Image for Contour filter on a uniform grid" << std::endl;

  vtkm::cont::ColorTable colorTable(
    { 0, 1 }, { 0.20f, 0.80f, .20f }, { .20f, .80f, .201f }, vtkm::ColorSpace::RGB);
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet inputData = maker.Make3DUniformDataSet0();
  std::string fieldName = "pointvar";
  VTKM_TEST_ASSERT(inputData.HasField(fieldName));

  vtkm::filter::Contour contour;
  contour.SetGenerateNormals(false);
  contour.SetMergeDuplicatePoints(true);
  contour.SetIsoValue(0, 100.0);
  contour.SetActiveField(fieldName);
  contour.SetFieldsToPass(fieldName);
  vtkm::cont::DataSet result = contour.Execute(inputData);

  result.PrintSummary(std::cout);

  //Y axis Flying Edge algorithm has subtle differences at a couple of boundaries
  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result, "pointvar", colorTable, "filter/contour-uniform.png", false);
}

void TestContourFilterTangle()
{
  std::cout << "Generate Image for Contour filter on a uniform tangle grid" << std::endl;

  vtkm::cont::ColorTable colorTable(
    { 0, 1 }, { 0.20f, 0.80f, .20f }, { .20f, .80f, .201f }, vtkm::ColorSpace::RGB);
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::Id3 dims(4, 4, 4);
  vtkm::source::Tangle tangle(dims);
  vtkm::cont::DataSet dataSet = tangle.Execute();

  vtkm::filter::Contour contour;
  contour.SetGenerateNormals(true);
  contour.SetIsoValue(0, 1);
  contour.SetActiveField("nodevar");
  contour.SetFieldsToPass("nodevar");
  auto result = contour.Execute(dataSet);

  result.PrintSummary(std::cout);

  //Y axis Flying Edge algorithm has subtle differences at a couple of boundaries
  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(
    result, "nodevar", colorTable, "filter/contour-tangle.png", false);
}

void TestContourFilter()
{
  TestContourFilterUniform();
  TestContourFilterTangle();
  TestContourFilterWedge();
}
} // namespace

int RegressionTestContourFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourFilter, argc, argv);
}
