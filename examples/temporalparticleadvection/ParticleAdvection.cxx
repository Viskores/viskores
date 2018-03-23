//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

#include <vtkm/cont/DataSet.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/ParticleAdvectionWorklets.h>
#include <vtkm/worklet/particleadvection/TemporalGridEvaluators.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/io/reader/BOVDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

// For offscreen rendering
/*#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
*/

#include <cstdlib>
#include <vector>

int renderAndWriteDataSet(vtkm::cont::DataSet& dataset)
{

  std::cout << "Trying to render the dataset" << std::endl;

  /*  vtkm::rendering::CanvasRayTracer canvas;
  vtkm::rendering::MapperWireframer mapper;

  vtkm::rendering::Actor actor(dataset.GetCellSet(),
                               dataset.GetCoordinateSystem(),
                               dataset.GetPointField(0),
                               vtkm::rendering::ColorTable("temperature"));
  vtkm::rendering::Scene scene;
  scene.AddActor(actor);

  // save images.
  vtkm::rendering::View3D view(scene, mapper, canvas);
  view.Initialize();
  view.SetBackgroundColor(vtkm::rendering::Color(1,1,1,1));
  view.SetForegroundColor(vtkm::rendering::Color(0,0,0,1));
  for(int i = 0; i < 16; i++)
  {
    std::ostringstream filename;
    filename << "clipped" << i << ".ppm";
    std::cout << "Writing " << filename.str() << std::endl;
    view.GetCamera().Azimuth(i*45.0);
    std::cout << "shifted " << std::endl;
    view.GetCamera().Elevation(i*45.0);
    std::cout << "lifted " << std::endl;
    view.Paint();
    std::cout << "painted" << std::endl;
    view.SaveAs(filename.str());
  }*/

  vtkm::io::writer::VTKDataSetWriter writer("vtkmwritten.vtk");
  writer.WriteDataSet(dataset, static_cast<vtkm::Id>(0));
  return 0;
}


void RunTest(vtkm::Id numSteps, vtkm::Float32 stepSize, vtkm::Id advectType)
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using FieldType = vtkm::Float32;
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>;
  using FieldPortalConstType =
    typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;

  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> fieldArray1;
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> fieldArray2;
  vtkm::Id numValues;

  vtkm::io::reader::BOVDataSetReader reader1("slice1.bov");
  vtkm::cont::DataSet ds1 = reader1.ReadDataSet();
  ds1.GetField(0).GetData().CopyTo(fieldArray1);

  vtkm::io::reader::BOVDataSetReader reader2("slice2.bov");
  vtkm::cont::DataSet ds2 = reader2.ReadDataSet();
  ds2.GetField(0).GetData().CopyTo(fieldArray2);

  using GridEvaluator =
    vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldPortalConstType,
                                                            FieldType,
                                                            DeviceAdapter>;
  using Integrator = vtkm::worklet::particleadvection::EulerIntegrator<GridEvaluator, FieldType>;


  //GridEvaluator eval(ds.GetCoordinateSystem(), ds.GetCellSet(0), fieldArray);
  GridEvaluator eval(ds1.GetCoordinateSystem(),
                     ds1.GetCellSet(0),
                     fieldArray1,
                     0,
                     ds2.GetCoordinateSystem(),
                     ds2.GetCellSet(0),
                     fieldArray2,
                     10.0);

  Integrator integrator(eval, stepSize);

  std::vector<vtkm::Vec<FieldType, 3>> seeds;
  vtkm::Id x = 0, y = 5, z = 0;
  for (int i = 0; i <= 11; i++)
  {
    vtkm::Vec<FieldType, 3> point;
    point[0] = x;
    point[1] = y;
    point[2] = z++;
    seeds.push_back(point);
  }

  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> seedArray;
  seedArray = vtkm::cont::make_ArrayHandle(seeds);

  if (advectType == 0)
  {
    vtkm::worklet::ParticleAdvection particleAdvection;
    particleAdvection.Run(integrator, seedArray, numSteps, DeviceAdapter());
  }
  else
  {
    vtkm::worklet::Streamline streamline;
    vtkm::worklet::StreamlineResult<FieldType> res =
      streamline.Run(integrator, seedArray, numSteps, DeviceAdapter());
    vtkm::cont::DataSet outData;
    vtkm::cont::CoordinateSystem outputCoords("coordinates", res.positions);
    outData.AddCellSet(res.polyLines);
    outData.AddCoordinateSystem(outputCoords);
    renderAndWriteDataSet(outData);
  }
}

int main(int argc, char** argv)
{
  vtkm::Id numSteps;
  vtkm::Float32 stepSize;
  vtkm::Id advectionType;

  if (argc < 4)
  {
    std::cout << "Wrong number of parameters provided" << std::endl;
    exit(EXIT_FAILURE);
  }

  numSteps = atoi(argv[1]);
  stepSize = atof(argv[2]);
  advectionType = atoi(argv[3]);

  RunTest(numSteps, stepSize, advectionType);

  return 0;
}
