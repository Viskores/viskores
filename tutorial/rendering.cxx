//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Initialize.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  //Loading .vtk File
  vtkm::io::VTKDataSetReader reader("data/kitchen.vtk");
  vtkm::cont::DataSet ds_from_file = reader.ReadDataSet();

  //Creating Actor
  vtkm::cont::ColorTable colorTable("viridis");
  vtkm::rendering::Actor actor(ds_from_file.GetCellSet(),
                               ds_from_file.GetCoordinateSystem(),
                               ds_from_file.GetField("c1"),
                               colorTable);

  //Creating Scene and adding Actor
  vtkm::rendering::Scene scene;
  scene.AddActor(actor);

  //Creating and initializing the View using the Canvas, Ray Tracer Mappers, and Scene
  vtkm::rendering::MapperRayTracer mapper;
  vtkm::rendering::CanvasRayTracer canvas(1920, 1080);
  vtkm::rendering::View3D view(scene, mapper, canvas);

  //Setting the background and foreground colors; optional.
  view.SetBackgroundColor(vtkm::rendering::Color(1.0f, 1.0f, 1.0f));
  view.SetForegroundColor(vtkm::rendering::Color(0.0f, 0.0f, 0.0f));

  //Painting View
  view.Paint();

  //Saving View
  view.SaveAs("BasicRendering.png");

  return 0;
}
