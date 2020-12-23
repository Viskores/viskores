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

#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/Contour.h>
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

  C canvas(512, 512);
  M mapper;
  vtkm::rendering::Scene scene;
  auto view = vtkm::rendering::testing::GetViewPtr<M, C, V3>(
    result, "gyroid", canvas, mapper, scene, colorTable, static_cast<vtkm::FloatDefault>(0.08));

  VTKM_TEST_ASSERT(test_equal_images_matching_name(view, "contour-wedge.png"));
}

void TestContourFilter()
{
  TestContourFilterWedge();
}
} // namespace

int RegressionTestContourFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourFilter, argc, argv);
}
