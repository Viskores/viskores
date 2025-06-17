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
#include <viskores/cont/ColorTable.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Initialize.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/rendering/Actor.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>

int main(int argc, char** argv)
{
  viskores::cont::Initialize(argc, argv);

  //Loading .vtk File
  viskores::io::VTKDataSetReader reader("data/kitchen.vtk");
  viskores::cont::DataSet ds_from_file = reader.ReadDataSet();

  //Creating Actor
  viskores::cont::ColorTable colorTable("viridis");
  viskores::rendering::Actor actor(ds_from_file, "c1", colorTable);

  //Creating Scene and adding Actor
  viskores::rendering::Scene scene;
  scene.AddActor(std::move(actor));

  //Creating and initializing the View using the Canvas, Ray Tracer Mappers, and Scene
  viskores::rendering::MapperRayTracer mapper;
  viskores::rendering::CanvasRayTracer canvas(1920, 1080);
  viskores::rendering::View3D view(scene, mapper, canvas);

  //Setting the background and foreground colors; optional.
  view.SetBackgroundColor(viskores::rendering::Color(1.0f, 1.0f, 1.0f));
  view.SetForegroundColor(viskores::rendering::Color(0.0f, 0.0f, 0.0f));

  //Painting View
  view.Paint();

  //Saving View
  view.SaveAs("BasicRendering.png");

  return 0;
}
