//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

#include <vtkm/io/reader/VTKDataSetReader.h>

#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/filter/MarchingCubes.h>

#include <iostream>

void makeScene(const vtkm::cont::DataSet& inputData,
               const vtkm::cont::ColorTable& colorTable,
               const std::string& fieldName,
               vtkm::rendering::Scene& scene)
{
  scene.AddActor(vtkm::rendering::Actor(inputData.GetCellSet(),
                                        inputData.GetCoordinateSystem(),
                                        inputData.GetField(fieldName),
                                        colorTable));
}

// This example reads an input vtk file specified on the command-line (or generates a default
// input data set if none is provided), uses VTK-m's rendering engine to render it to an
// output file using OS Mesa, instantiates an isosurface filter using VTK-m's filter
// mechanism, computes an isosurface on the input data set, packages the output of the filter
// in a new data set, and renders this output data set in a separate iamge file, again
// using VTK-m's rendering engine with OS Mesa.

int main(int argc, char* argv[])
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  // Input variable declarations
  vtkm::cont::DataSet inputData;
  vtkm::Float32 isovalue;
  std::string fieldName;

  // Get input data from specified file, or generate test data set
  if (argc < 3)
  {
    vtkm::cont::testing::MakeTestDataSet maker;
    inputData = maker.Make3DUniformDataSet0();
    isovalue = 100.0f;
    fieldName = "pointvar";
  }
  else
  {
    std::cout << "using: " << argv[1] << " as MarchingCubes input file" << std::endl;
    vtkm::io::reader::VTKDataSetReader reader(argv[1]);
    inputData = reader.ReadDataSet();
    isovalue = static_cast<vtkm::Float32>(atof(argv[2]));
    fieldName = "pointvar";
  }

  using Mapper = vtkm::rendering::MapperRayTracer;
  using Canvas = vtkm::rendering::CanvasRayTracer;

  // Set up a camera for rendering the input data
  const vtkm::cont::CoordinateSystem coords = inputData.GetCoordinateSystem();
  Mapper mapper;
  vtkm::rendering::Camera camera = vtkm::rendering::Camera();

  //Set3DView
  vtkm::Bounds coordsBounds = coords.GetBounds();

  camera.ResetToBounds(coordsBounds);

  vtkm::Vec<vtkm::Float32, 3> totalExtent;
  totalExtent[0] = vtkm::Float32(coordsBounds.X.Max - coordsBounds.X.Min);
  totalExtent[1] = vtkm::Float32(coordsBounds.Y.Max - coordsBounds.Y.Min);
  totalExtent[2] = vtkm::Float32(coordsBounds.Z.Max - coordsBounds.Z.Min);
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

  makeScene(inputData, colorTable, fieldName, scene);
  // Create a view and use it to render the input data using OS Mesa
  vtkm::rendering::View3D view(scene, mapper, canvas, camera, bg);
  view.Initialize();
  view.Paint();
  view.SaveAs("demo_input.pnm");

  // Create an isosurface filter
  vtkm::filter::MarchingCubes filter;
  filter.SetGenerateNormals(false);
  filter.SetMergeDuplicatePoints(false);
  filter.SetIsoValue(0, isovalue);
  filter.SetActiveField(fieldName);
  vtkm::cont::DataSet outputData = filter.Execute(inputData);
  // Render a separate image with the output isosurface
  std::cout << "about to render the results of the MarchingCubes filter" << std::endl;
  vtkm::rendering::Scene scene2;
  makeScene(outputData, colorTable, fieldName, scene2);

  vtkm::rendering::View3D view2(scene2, mapper, canvas, camera, bg);
  view2.Initialize();
  view2.Paint();
  view2.SaveAs("demo_output.pnm");

  return 0;
}
