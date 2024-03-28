//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

////
//// BEGIN-EXAMPLE VTKmQuickStart
////
#include <vtkm/cont/Initialize.h>

#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/filter/mesh_info/MeshQuality.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

////
//// BEGIN-EXAMPLE VTKmQuickStartInitialize
////
int main(int argc, char* argv[])
{
  vtkm::cont::Initialize(argc, argv, vtkm::cont::InitializeOptions::AddHelp);
  ////
  //// END-EXAMPLE VTKmQuickStartInitialize
  ////

  if (argc != 2)
  {
    std::cerr << "USAGE: " << argv[0] << " <file.vtk>" << std::endl;
    return 1;
  }

  // Read in a file specified in the first command line argument.
  ////
  //// BEGIN-EXAMPLE VTKmQuickStartReadFile
  ////
  vtkm::io::VTKDataSetReader reader(argv[1]);
  vtkm::cont::DataSet inData = reader.ReadDataSet();
  ////
  //// END-EXAMPLE VTKmQuickStartReadFile
  ////
  //// PAUSE-EXAMPLE
  inData.PrintSummary(std::cout);
  //// RESUME-EXAMPLE

  // Run the data through the elevation filter.
  ////
  //// BEGIN-EXAMPLE VTKmQuickStartFilter
  ////
  vtkm::filter::mesh_info::MeshQuality cellArea;
  cellArea.SetMetric(vtkm::filter::mesh_info::CellMetric::Area);
  vtkm::cont::DataSet outData = cellArea.Execute(inData);
  ////
  //// END-EXAMPLE VTKmQuickStartFilter
  ////

  // Render an image and write it out to a file.
  ////
  //// BEGIN-EXAMPLE VTKmQuickStartRender
  ////
  //// LABEL scene-start
  vtkm::rendering::Actor actor(
    outData.GetCellSet(), outData.GetCoordinateSystem(), outData.GetField("area"));

  vtkm::rendering::Scene scene;
  //// LABEL scene-end
  scene.AddActor(actor);

  vtkm::rendering::MapperRayTracer mapper;

  vtkm::rendering::CanvasRayTracer canvas(1280, 1024);

  //// LABEL view
  vtkm::rendering::View3D view(scene, mapper, canvas);

  //// LABEL paint
  view.Paint();

  //// LABEL save
  view.SaveAs("image.png");
  ////
  //// END-EXAMPLE VTKmQuickStartRender
  ////

  return 0;
}
////
//// END-EXAMPLE VTKmQuickStart
////
