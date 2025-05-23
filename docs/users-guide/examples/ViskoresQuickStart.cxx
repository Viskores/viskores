//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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
//// BEGIN-EXAMPLE ViskoresQuickStart
////
#include <viskores/cont/Initialize.h>

#include <viskores/io/VTKDataSetReader.h>

#include <viskores/filter/mesh_info/MeshQuality.h>

#include <viskores/rendering/Actor.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>

////
//// BEGIN-EXAMPLE ViskoresQuickStartInitialize
////
int main(int argc, char* argv[])
{
  viskores::cont::Initialize(argc, argv, viskores::cont::InitializeOptions::AddHelp);
  ////
  //// END-EXAMPLE ViskoresQuickStartInitialize
  ////

  if (argc != 2)
  {
    std::cerr << "USAGE: " << argv[0] << " <file.vtk>" << std::endl;
    return 1;
  }

  // Read in a file specified in the first command line argument.
  ////
  //// BEGIN-EXAMPLE ViskoresQuickStartReadFile
  ////
  viskores::io::VTKDataSetReader reader(argv[1]);
  viskores::cont::DataSet inData = reader.ReadDataSet();
  ////
  //// END-EXAMPLE ViskoresQuickStartReadFile
  ////
  //// PAUSE-EXAMPLE
  inData.PrintSummary(std::cout);
  //// RESUME-EXAMPLE

  // Run the data through the elevation filter.
  ////
  //// BEGIN-EXAMPLE ViskoresQuickStartFilter
  ////
  viskores::filter::mesh_info::MeshQuality cellArea;
  cellArea.SetMetric(viskores::filter::mesh_info::CellMetric::Area);
  viskores::cont::DataSet outData = cellArea.Execute(inData);
  ////
  //// END-EXAMPLE ViskoresQuickStartFilter
  ////

  // Render an image and write it out to a file.
  ////
  //// BEGIN-EXAMPLE ViskoresQuickStartRender
  ////
  //// LABEL scene-start
  viskores::rendering::Actor actor(outData, "area");

  viskores::rendering::Scene scene;
  //// LABEL scene-end
  scene.AddActor(actor);

  viskores::rendering::MapperRayTracer mapper;

  viskores::rendering::CanvasRayTracer canvas(1280, 1024);

  //// LABEL view
  viskores::rendering::View3D view(scene, mapper, canvas);

  //// LABEL paint
  view.Paint();

  //// LABEL save
  view.SaveAs("image.png");
  ////
  //// END-EXAMPLE ViskoresQuickStartRender
  ////

  return 0;
}
////
//// END-EXAMPLE ViskoresQuickStart
////
