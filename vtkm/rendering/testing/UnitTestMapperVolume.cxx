//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/field_conversion/CellAverage.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/source/Tangle.h>

namespace
{

void TestRectilinear()
{
  vtkm::cont::ColorTable colorTable = vtkm::cont::ColorTable::Preset::Inferno;
  colorTable.AddPointAlpha(0.0, 0.01f);
  colorTable.AddPointAlpha(0.4, 0.01f);
  colorTable.AddPointAlpha(0.7, 0.2f);
  colorTable.AddPointAlpha(1.0, 0.5f);

  vtkm::rendering::testing::RenderTestOptions options;
  options.Mapper = vtkm::rendering::testing::MapperType::Volume;
  options.AllowAnyDevice = false;
  options.ColorTable = colorTable;

  vtkm::cont::DataSet rectDS, unsDS;
  std::string rectfname = vtkm::cont::testing::Testing::DataPath("third_party/visit/example.vtk");
  vtkm::io::VTKDataSetReader rectReader(rectfname);

  try
  {
    rectDS = rectReader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += rectfname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  vtkm::rendering::testing::RenderTest(
    rectDS, "temp", "rendering/volume/rectilinear3D.png", options);

  vtkm::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("temp");
  cellAverage.SetOutputFieldName("temp_avg");
  vtkm::cont::DataSet tempAvg = cellAverage.Execute(rectDS);

  vtkm::rendering::testing::RenderTest(
    tempAvg, "temp_avg", "rendering/volume/rectilinear3D_cell.png", options);
}

void TestUniformGrid()
{
  vtkm::cont::ColorTable colorTable = vtkm::cont::ColorTable::Preset::Inferno;
  colorTable.AddPointAlpha(0.0, 0.2f);
  colorTable.AddPointAlpha(0.2, 0.0f);
  colorTable.AddPointAlpha(0.5, 0.0f);

  vtkm::rendering::testing::RenderTestOptions options;
  options.Mapper = vtkm::rendering::testing::MapperType::Volume;
  options.AllowAnyDevice = false;
  options.ColorTable = colorTable;
  // Rendering of AxisAnnotation3D is sensitive on the type
  // of FloatDefault, disable it before we know how to fix
  // it properly.
  options.EnableAnnotations = false;

  vtkm::source::Tangle tangle;
  tangle.SetPointDimensions({ 50, 50, 50 });
  vtkm::cont::DataSet tangleData = tangle.Execute();

  vtkm::rendering::testing::RenderTest(
    tangleData, "tangle", "rendering/volume/uniform.png", options);

  vtkm::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("tangle");
  cellAverage.SetOutputFieldName("tangle_avg");
  vtkm::cont::DataSet tangleAvg = cellAverage.Execute(tangleData);

  vtkm::rendering::testing::RenderTest(
    tangleAvg, "tangle_avg", "rendering/volume/uniform_cell.png", options);
}

void RenderTests()
{
  TestRectilinear();
  TestUniformGrid();
}

} //namespace

int UnitTestMapperVolume(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
