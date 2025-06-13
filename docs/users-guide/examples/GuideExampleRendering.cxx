//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/io/VTKDataSetReader.h>

#include <viskores/rendering/Actor.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/GlyphType.h>
#include <viskores/rendering/MapperGlyphScalar.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/MapperWireframer.h>
#include <viskores/rendering/View3D.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

void DoBasicRender()
{
  // Load some data to render
  viskores::cont::DataSet surfaceData;
  try
  {
    viskores::io::VTKDataSetReader reader(
      viskores::cont::testing::Testing::GetTestDataBasePath() + "unstructured/cow.vtk");
    surfaceData = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& error)
  {
    std::cout << "Could not read file:" << std::endl << error.GetMessage() << std::endl;
    exit(1);
  }
  catch (...)
  {
    throw;
  }

  // Initialize Viskores rendering classes
  ////
  //// BEGIN-EXAMPLE ConstructView
  ////
  ////
  //// BEGIN-EXAMPLE ActorScene
  ////
  viskores::rendering::Actor actor(surfaceData, "RandomPointScalars");

  viskores::rendering::Scene scene;
  scene.AddActor(actor);
  ////
  //// END-EXAMPLE ActorScene
  ////

  viskores::rendering::MapperRayTracer mapper;
  ////
  //// BEGIN-EXAMPLE Canvas
  ////
  viskores::rendering::CanvasRayTracer canvas(1920, 1080);
  ////
  //// END-EXAMPLE Canvas
  ////

  viskores::rendering::View3D view(scene, mapper, canvas);
  ////
  //// END-EXAMPLE ConstructView
  ////

  ////
  //// BEGIN-EXAMPLE ViewColors
  ////
  view.SetBackgroundColor(viskores::rendering::Color(1.0f, 1.0f, 1.0f));
  view.SetForegroundColor(viskores::rendering::Color(0.0f, 0.0f, 0.0f));
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
  viskores::cont::DataSet surfaceData;
  try
  {
    viskores::io::VTKDataSetReader reader(
      viskores::cont::testing::Testing::GetTestDataBasePath() + "unstructured/cow.vtk");
    surfaceData = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& error)
  {
    std::cout << "Could not read file:" << std::endl << error.GetMessage() << std::endl;
    exit(1);
  }
  catch (...)
  {
    throw;
  }

  // Initialize Viskores rendering classes
  viskores::rendering::Actor actor(surfaceData, "RandomPointScalars");

  viskores::rendering::Scene scene;
  scene.AddActor(actor);

  viskores::rendering::CanvasRayTracer canvas(1920, 1080);

  ////
  //// BEGIN-EXAMPLE MapperGlyphScalar
  ////
  viskores::rendering::MapperGlyphScalar mapper;
  mapper.SetGlyphType(viskores::rendering::GlyphType::Cube);
  mapper.SetScaleByValue(true);
  mapper.SetScaleDelta(10.0f);

  viskores::rendering::View3D view(scene, mapper, canvas);
  ////
  //// END-EXAMPLE MapperGlyphScalar
  ////

  view.SetBackgroundColor(viskores::rendering::Color(1.0f, 1.0f, 1.0f));
  view.SetForegroundColor(viskores::rendering::Color(0.0f, 0.0f, 0.0f));

  view.Paint();

  view.SaveAs("GlyphRendering.ppm");
}

void DoEdgeRender()
{
  // Load some data to render
  viskores::cont::DataSet surfaceData;
  try
  {
    viskores::io::VTKDataSetReader reader(
      viskores::cont::testing::Testing::GetTestDataBasePath() + "unstructured/cow.vtk");
    surfaceData = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& error)
  {
    std::cout << "Could not read file:" << std::endl << error.GetMessage() << std::endl;
    exit(1);
  }
  catch (...)
  {
    throw;
  }

  // Initialize Viskores rendering classes
  viskores::rendering::Actor actor(surfaceData, "RandomPointScalars");

  viskores::rendering::Scene scene;
  scene.AddActor(actor);

  viskores::rendering::CanvasRayTracer canvas(1920, 1080);

  ////
  //// BEGIN-EXAMPLE MapperEdge
  ////
  viskores::rendering::MapperWireframer mapper;
  viskores::rendering::View3D view(scene, mapper, canvas);
  ////
  //// END-EXAMPLE MapperEdge
  ////

  view.SetBackgroundColor(viskores::rendering::Color(1.0f, 1.0f, 1.0f));
  view.SetForegroundColor(viskores::rendering::Color(0.0f, 0.0f, 0.0f));

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
  return viskores::cont::testing::Testing::Run(DoRender, argc, argv);
}
