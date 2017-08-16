
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <random>

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/rendering/CanvasLineRenderer.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperLineRenderer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderCow()
{
  auto dataSet = vtkm::io::reader::VTKDataSetReader("hardy_global.vtk").ReadDataSet();
  const char* fieldName = "colorVar";
  dataSet.PrintSummary(std::cout);

  vtkm::rendering::MapperLineRenderer mapper;
  vtkm::rendering::CanvasLineRenderer canvas(1024, 1024);
  vtkm::rendering::Scene scene;
  scene.AddActor(vtkm::rendering::Actor(dataSet.GetCellSet(),
                                        dataSet.GetCoordinateSystem(),
                                        dataSet.GetField(fieldName),
                                        vtkm::rendering::ColorTable("thermal")));

  vtkm::rendering::Camera camera;
  vtkm::Bounds coordsBounds = dataSet.GetCoordinateSystem().GetBounds();
  vtkm::Vec<vtkm::Float64, 3> totalExtent(
    coordsBounds.X.Length(), coordsBounds.Y.Length(), coordsBounds.Z.Length());
  vtkm::Normalize(totalExtent);
  camera.ResetToBounds(dataSet.GetCoordinateSystem().GetBounds());
  camera.SetFieldOfView(45.0f);

  vtkm::rendering::View3D view(scene, mapper, canvas, camera, vtkm::rendering::Color::white);
  view.Initialize();
  //mapper.SetShowInternalZones(false);
  //view.Paint();
  //view.SaveAs("hardy_global_external.pnm");
  mapper.SetShowInternalZones(true);
  view.Paint();
  view.SaveAs("hardy_global_internal.pnm");
}

void RenderBunny()
{
  auto dataSet = vtkm::io::reader::VTKDataSetReader("bunny.vtk").ReadDataSet();
  const char* fieldName = "RandomPointScalars";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0.0f, 1.0f);
  std::vector<vtkm::Float32> scalarField;
  for (vtkm::Id i = 0; i < dataSet.GetCoordinateSystem().GetData().GetNumberOfValues(); ++i)
  {
    scalarField.push_back(dis(gen));
  }
  vtkm::cont::DataSetFieldAdd fa;
  fa.AddPointField(dataSet, fieldName, scalarField);
  vtkm::rendering::MapperLineRenderer mapper;
  vtkm::rendering::CanvasLineRenderer canvas(1024, 1024);
  vtkm::rendering::MapperRayTracer mapper2;
  vtkm::rendering::CanvasRayTracer canvas2(1024, 1024);
  vtkm::rendering::Scene scene;
  scene.AddActor(vtkm::rendering::Actor(dataSet.GetCellSet(),
                                        dataSet.GetCoordinateSystem(),
                                        dataSet.GetField(fieldName),
                                        vtkm::rendering::ColorTable("thermal")));

  vtkm::rendering::Camera camera;
  vtkm::Bounds coordsBounds = dataSet.GetCoordinateSystem().GetBounds();
  vtkm::Vec<vtkm::Float64, 3> totalExtent(
    coordsBounds.X.Length(), coordsBounds.Y.Length(), coordsBounds.Z.Length());
  vtkm::Normalize(totalExtent);
  camera.ResetToBounds(dataSet.GetCoordinateSystem().GetBounds());
  camera.SetFieldOfView(45.0f);

  vtkm::rendering::View3D view(scene, mapper, canvas, camera, vtkm::rendering::Color::white);
  view.Initialize();
  mapper.SetShowInternalZones(false);
  view.Paint();
  view.SaveAs("bunny_external.pnm");
  vtkm::rendering::View3D view2(scene, mapper2, canvas2, camera, vtkm::rendering::Color::white);
  view2.Initialize();
  view2.Paint();
  view2.SaveAs("bunny_ray.pnm");
}

void RenderTests()
{
  typedef vtkm::rendering::MapperLineRenderer M;
  typedef vtkm::rendering::CanvasLineRenderer C;
  typedef vtkm::rendering::View3D V3;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::rendering::ColorTable colorTable("thermal");

  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRegularDataSet0(), "pointvar", colorTable, "reg3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "rect3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "expl3D.pnm");
}

} //namespace

int UnitTestMapperLineRenderer(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RenderTests) |
    vtkm::cont::testing::Testing::Run(RenderCow);
}
