//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

#include <vtkm/filter/Contour.h>

// This example creates a simple data set and uses VTK-m's rendering engine to render an image and
// write that image to a file. It then computes an isosurface on the input data set and renders
// this output data set in a separate image file

int main(int argc, char* argv[])
{
  vtkm::cont::Initialize(argc, argv, vtkm::cont::InitializeOptions::Strict);

  // Input variable declarations
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet inputData = maker.Make3DUniformDataSet0();
  vtkm::Float32 isovalue = 100.0f;
  std::string fieldName = "pointvar";

  using Mapper = vtkm::rendering::MapperRayTracer;
  using Canvas = vtkm::rendering::CanvasRayTracer;

  // Set up a camera for rendering the input data
  const vtkm::cont::CoordinateSystem coords = inputData.GetCoordinateSystem();
  Mapper mapper;
  vtkm::rendering::Camera camera;

  //Set3DView
  vtkm::Bounds coordsBounds = coords.GetBounds();

  camera.ResetToBounds(coordsBounds);

  vtkm::Vec3f_32 totalExtent;
  totalExtent[0] = vtkm::Float32(coordsBounds.X.Length());
  totalExtent[1] = vtkm::Float32(coordsBounds.Y.Length());
  totalExtent[2] = vtkm::Float32(coordsBounds.Z.Length());
  vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
  vtkm::Normalize(totalExtent);
  camera.SetLookAt(totalExtent * (mag * .5f));
  camera.SetViewUp(vtkm::make_Vec(0.f, 1.f, 0.f));
  camera.SetClippingRange(1.f, 100.f);
  camera.SetFieldOfView(60.f);
  camera.SetPosition(totalExtent * (mag * 2.f));
  vtkm::cont::ColorTable colorTable("inferno");

  // Create a scene for rendering the input data
  vtkm::rendering::Scene scene;
  vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  Canvas canvas(512, 512);

  vtkm::rendering::Actor actor(inputData.GetCellSet(),
                               inputData.GetCoordinateSystem(),
                               inputData.GetField(fieldName),
                               colorTable);
  scene.AddActor(actor);
  // Create a view and use it to render the input data using OS Mesa
  vtkm::rendering::View3D view(scene, mapper, canvas, camera, bg);
  view.Initialize();
  view.Paint();
  view.SaveAs("demo_input.pnm");

  // Create an isosurface filter
  vtkm::filter::Contour filter;
  filter.SetGenerateNormals(false);
  filter.SetMergeDuplicatePoints(false);
  filter.SetIsoValue(0, isovalue);
  filter.SetActiveField(fieldName);
  vtkm::cont::DataSet outputData = filter.Execute(inputData);
  // Render a separate image with the output isosurface
  std::cout << "about to render the results of the Contour filter" << std::endl;
  vtkm::rendering::Scene scene2;
  vtkm::rendering::Actor actor2(outputData.GetCellSet(),
                                outputData.GetCoordinateSystem(),
                                outputData.GetField(fieldName),
                                colorTable);
  // By default, the actor will automatically scale the scalar range of the color table to match
  // that of the data. However, we are coloring by the scalar that we just extracted a contour
  // from, so we want the scalar range to match that of the previous image.
  actor2.SetScalarRange(actor.GetScalarRange());
  scene2.AddActor(actor2);

  vtkm::rendering::View3D view2(scene2, mapper, canvas, camera, bg);
  view2.Initialize();
  view2.Paint();
  view2.SaveAs("demo_output.pnm");

  return 0;
}
