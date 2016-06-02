//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/rendering/MapperGL.h>
#include <vtkm/rendering/Plot.h>
#include <vtkm/rendering/RenderSurfaceOSMesa.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/Window.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/io/reader/VTKDataSetReader.h>

#include <vtkm/filter/MarchingCubes.h>
#include <vtkm/cont/DataSetFieldAdd.h>

#include <iostream>

// This example reads an input vtk file specified on the command-line (or generates a default
// input data set if none is provided), uses VTK-m's rendering engine to render it to an
// output file using OS Mesa, instantiates an isosurface filter using VTK-m's filter
// mechanism, computes an isosurface on the input data set, packages the output of the filter
// in a new data set, and renders this output data set in a separate iamge file, again
// using VTK-m's rendering engine with OS Mesa.

int main(int argc, char* argv[])
{
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
    isovalue = atof(argv[2]);
    fieldName = "SCALARS:pointvar";
  }

  typedef vtkm::rendering::MapperGL< > Mapper;
  typedef vtkm::rendering::RenderSurfaceOSMesa RenderSurface;

  // Set up a view for rendering the input data
  const vtkm::cont::CoordinateSystem coords = inputData.GetCoordinateSystem();
  Mapper mapper;
  vtkm::rendering::View3D &view = mapper.GetView();
  vtkm::Float64 coordsBounds[6];
  coords.GetBounds(coordsBounds,VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
  vtkm::Vec<vtkm::Float32,3> totalExtent;
  totalExtent[0] = vtkm::Float32(coordsBounds[1] - coordsBounds[0]);
  totalExtent[1] = vtkm::Float32(coordsBounds[3] - coordsBounds[2]);
  totalExtent[2] = vtkm::Float32(coordsBounds[5] - coordsBounds[4]);
  vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
  vtkm::Normalize(totalExtent);
  view.LookAt = totalExtent * (mag * .5f);
  view.Up = vtkm::make_Vec(0.f, 1.f, 0.f);
  view.NearPlane = 1.f;
  view.FarPlane = 100.f;
  view.FieldOfView = 60.f;
  view.Height = 512;
  view.Width = 512;
  view.Position = totalExtent * (mag * 2.f);
  vtkm::rendering::ColorTable colorTable("thermal");
  mapper.SetActiveColorTable(colorTable);
  mapper.SetView(view);

  // Create a scene for rendering the input data
  vtkm::rendering::Scene3D scene;
  vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  vtkm::rendering::RenderSurfaceOSMesa surface(512,512,bg);
  scene.plots.push_back(vtkm::rendering::Plot(inputData.GetCellSet(),
                                              inputData.GetCoordinateSystem(),
                                              inputData.GetField(fieldName),
                                              colorTable));

  // Create a window and use it to render the input data using OS Mesa
  vtkm::rendering::Window3D<Mapper, RenderSurface> w1(scene,
                                                      mapper,
                                                      surface,
                                                      bg);
  w1.Initialize();
  w1.Paint();
  w1.SaveAs("demo_input.pnm");

  // Create an isosurface filter
  vtkm::filter::MarchingCubes filter;
  filter.SetGenerateNormals(false);
  filter.SetMergeDuplicatePoints(false);
  filter.SetIsoValue(isovalue);
  vtkm::filter::DataSetResult result = filter.Execute( inputData,
                                                       inputData.GetField(fieldName) );
  filter.MapFieldOntoOutput(result, inputData.GetField(fieldName));
  vtkm::cont::DataSet& outputData = result.GetDataSet();
  // Render a separate image with the output isosurface
  std::cout << "about to plot the results of the MarchingCubes filter" << std::endl;
  scene.plots.clear();
  scene.plots.push_back(vtkm::rendering::Plot(outputData.GetCellSet(),
                                              outputData.GetCoordinateSystem(),
                                              outputData.GetField(fieldName),
                                              colorTable));

  vtkm::rendering::Window3D< Mapper, RenderSurface> w2(scene,
                                                              mapper,
                                                              surface,
                                                              bg);
  w2.Initialize();
  w2.Paint();
  w2.SaveAs("demo_output.pnm");

  return 0;
}
