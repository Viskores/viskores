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

#include <viskores/cont/Initialize.h>
#include <viskores/source/Tangle.h>

#include <viskores/rendering/Actor.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/MapperVolume.h>
#include <viskores/rendering/MapperWireframer.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>

#include <viskores/filter/contour/Contour.h>

// This example creates a simple data set and uses Viskores's rendering engine to render an image and
// write that image to a file. It then computes an isosurface on the input data set and renders
// this output data set in a separate image file

using viskores::rendering::CanvasRayTracer;
using viskores::rendering::MapperRayTracer;
using viskores::rendering::MapperVolume;
using viskores::rendering::MapperWireframer;

int main(int argc, char* argv[])
{
  viskores::cont::Initialize(argc, argv, viskores::cont::InitializeOptions::Strict);

  viskores::source::Tangle tangle;
  tangle.SetPointDimensions({ 50, 50, 50 });
  viskores::cont::DataSet tangleData = tangle.Execute();
  std::string fieldName = "tangle";

  // Set up a camera for rendering the input data
  viskores::rendering::Camera camera;
  camera.SetLookAt(viskores::Vec3f_32(0.5, 0.5, 0.5));
  camera.SetViewUp(viskores::make_Vec(0.f, 1.f, 0.f));
  camera.SetClippingRange(1.f, 10.f);
  camera.SetFieldOfView(60.f);
  camera.SetPosition(viskores::Vec3f_32(1.5, 1.5, 1.5));
  viskores::cont::ColorTable colorTable("inferno");

  // Background color:
  viskores::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  viskores::rendering::Actor actor(tangleData, fieldName, colorTable);
  viskores::rendering::Scene scene;
  scene.AddActor(actor);
  // 2048x2048 pixels in the canvas:
  CanvasRayTracer canvas(2048, 2048);
  // Create a view and use it to render the input data using OS Mesa

  viskores::rendering::View3D view(scene, MapperVolume(), canvas, camera, bg);
  view.Paint();
  view.SaveAs("volume.png");

  // Compute an isosurface:
  viskores::filter::contour::Contour filter;
  // [min, max] of the tangle field is [-0.887, 24.46]:
  filter.SetIsoValue(3.0);
  filter.SetActiveField(fieldName);
  viskores::cont::DataSet isoData = filter.Execute(tangleData);
  // Render a separate image with the output isosurface
  viskores::rendering::Actor isoActor(isoData, fieldName, colorTable);
  // By default, the actor will automatically scale the scalar range of the
  // color table to match that of the data. However, we are coloring by the
  // scalar that we just extracted a contour from, so we want the scalar range
  // to match that of the previous image.
  isoActor.SetScalarRange(actor.GetScalarRange());
  viskores::rendering::Scene isoScene;
  isoScene.AddActor(std::move(isoActor));

  // Wireframe surface:
  viskores::rendering::View3D isoView(isoScene, MapperWireframer(), canvas, camera, bg);
  isoView.Paint();
  isoView.SaveAs("isosurface_wireframer.png");

  // Smooth surface:
  viskores::rendering::View3D solidView(isoScene, MapperRayTracer(), canvas, camera, bg);
  solidView.Paint();
  solidView.SaveAs("isosurface_raytracer.png");

  return 0;
}
