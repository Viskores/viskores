//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/GlyphType.h>
#include <vtkm/rendering/MapperGlyphScalar.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/View3D.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

void DoBasicRender()
{
  // Load some data to render
  vtkm::cont::DataSet surfaceData;
  try
  {
    vtkm::io::VTKDataSetReader reader(
      vtkm::cont::testing::Testing::GetTestDataBasePath() + "unstructured/cow.vtk");
    surfaceData = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& error)
  {
    std::cout << "Could not read file:" << std::endl << error.GetMessage() << std::endl;
    exit(1);
  }
  catch (...)
  {
    throw;
  }

  // Initialize VTK-m rendering classes
  ////
  //// BEGIN-EXAMPLE ConstructView
  ////
  ////
  //// BEGIN-EXAMPLE ActorScene
  ////
  vtkm::rendering::Actor actor(surfaceData.GetCellSet(),
                               surfaceData.GetCoordinateSystem(),
                               surfaceData.GetField("RandomPointScalars"));

  vtkm::rendering::Scene scene;
  scene.AddActor(actor);
  ////
  //// END-EXAMPLE ActorScene
  ////

  vtkm::rendering::MapperRayTracer mapper;
  ////
  //// BEGIN-EXAMPLE Canvas
  ////
  vtkm::rendering::CanvasRayTracer canvas(1920, 1080);
  ////
  //// END-EXAMPLE Canvas
  ////

  vtkm::rendering::View3D view(scene, mapper, canvas);
  ////
  //// END-EXAMPLE ConstructView
  ////

  ////
  //// BEGIN-EXAMPLE ViewColors
  ////
  view.SetBackgroundColor(vtkm::rendering::Color(1.0f, 1.0f, 1.0f));
  view.SetForegroundColor(vtkm::rendering::Color(0.0f, 0.0f, 0.0f));
  ////
  //// END-EXAMPLE ViewColors
  ////

  ////
  //// BEGIN-EXAMPLE PaintView
  ////
  view.Paint();
  ////
  //// END-EXAMPLE PaintView
  ////

  ////
  //// BEGIN-EXAMPLE SaveView
  ////
  view.SaveAs("BasicRendering.png");
  ////
  //// END-EXAMPLE SaveView
  ////
}

void DoPointRender()
{
  // Load some data to render
  vtkm::cont::DataSet surfaceData;
  try
  {
    vtkm::io::VTKDataSetReader reader(
      vtkm::cont::testing::Testing::GetTestDataBasePath() + "unstructured/cow.vtk");
    surfaceData = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& error)
  {
    std::cout << "Could not read file:" << std::endl << error.GetMessage() << std::endl;
    exit(1);
  }
  catch (...)
  {
    throw;
  }

  // Initialize VTK-m rendering classes
  vtkm::rendering::Actor actor(surfaceData.GetCellSet(),
                               surfaceData.GetCoordinateSystem(),
                               surfaceData.GetField("RandomPointScalars"));

  vtkm::rendering::Scene scene;
  scene.AddActor(actor);

  vtkm::rendering::CanvasRayTracer canvas(1920, 1080);

  ////
  //// BEGIN-EXAMPLE MapperGlyphScalar
  ////
  vtkm::rendering::MapperGlyphScalar mapper;
  mapper.SetGlyphType(vtkm::rendering::GlyphType::Cube);
  mapper.SetScaleByValue(true);
  mapper.SetScaleDelta(10.0f);

  vtkm::rendering::View3D view(scene, mapper, canvas);
  ////
  //// END-EXAMPLE MapperGlyphScalar
  ////

  view.SetBackgroundColor(vtkm::rendering::Color(1.0f, 1.0f, 1.0f));
  view.SetForegroundColor(vtkm::rendering::Color(0.0f, 0.0f, 0.0f));

  view.Paint();

  view.SaveAs("GlyphRendering.ppm");
}

void DoEdgeRender()
{
  // Load some data to render
  vtkm::cont::DataSet surfaceData;
  try
  {
    vtkm::io::VTKDataSetReader reader(
      vtkm::cont::testing::Testing::GetTestDataBasePath() + "unstructured/cow.vtk");
    surfaceData = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& error)
  {
    std::cout << "Could not read file:" << std::endl << error.GetMessage() << std::endl;
    exit(1);
  }
  catch (...)
  {
    throw;
  }

  // Initialize VTK-m rendering classes
  vtkm::rendering::Actor actor(surfaceData.GetCellSet(),
                               surfaceData.GetCoordinateSystem(),
                               surfaceData.GetField("RandomPointScalars"));

  vtkm::rendering::Scene scene;
  scene.AddActor(actor);

  vtkm::rendering::CanvasRayTracer canvas(1920, 1080);

  ////
  //// BEGIN-EXAMPLE MapperEdge
  ////
  vtkm::rendering::MapperWireframer mapper;
  vtkm::rendering::View3D view(scene, mapper, canvas);
  ////
  //// END-EXAMPLE MapperEdge
  ////

  view.SetBackgroundColor(vtkm::rendering::Color(1.0f, 1.0f, 1.0f));
  view.SetForegroundColor(vtkm::rendering::Color(0.0f, 0.0f, 0.0f));

  view.Paint();

  view.SaveAs("EdgeRendering.png");
}

void DoRender()
{
  DoBasicRender();
  DoPointRender();
  DoEdgeRender();
}

} // anonymous namespace

int GuideExampleRendering(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoRender, argc, argv);
}
