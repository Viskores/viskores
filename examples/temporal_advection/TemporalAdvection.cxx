//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Initialize.h>

#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/ParticleAdvectionWorklets.h>
#include <vtkm/worklet/particleadvection/TemporalGridEvaluators.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/io/reader/BOVDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

#include <cstdlib>
#include <vector>

// The way to verify if the code produces correct streamlines
// is to do a visual test by using VisIt/ParaView to visualize
// the file written by this method.
int renderAndWriteDataSet(const vtkm::cont::DataSet& dataset)
{
  std::cout << "Trying to render the dataset" << std::endl;
  vtkm::io::writer::VTKDataSetWriter writer("pathlines.vtk");
  writer.WriteDataSet(dataset);
  return 0;
}


void RunTest(vtkm::Id numSteps, vtkm::Float32 stepSize, vtkm::Id advectType)
{
  using FieldType = vtkm::Float32;
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>;

  // These lines read two datasets, which are BOVs.
  // Currently VTKm does not support providing time series datasets
  // In the hackiest way possible this pathlines example run using
  // two time slices. The way to provide more datasets will be explored
  // as VTKm evolves.
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> fieldArray1;
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> fieldArray2;

  vtkm::io::reader::BOVDataSetReader reader1("slice1.bov");
  vtkm::cont::DataSet ds1 = reader1.ReadDataSet();
  ds1.GetField(0).GetData().CopyTo(fieldArray1);

  vtkm::io::reader::BOVDataSetReader reader2("slice2.bov");
  vtkm::cont::DataSet ds2 = reader2.ReadDataSet();
  ds2.GetField(0).GetData().CopyTo(fieldArray2);

  // The only change in this example and the vanilla particle advection example is
  // this example makes use of the TemporalGridEvaluator.
  using GridEvaluator = vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldHandle>;
  using Integrator = vtkm::worklet::particleadvection::EulerIntegrator<GridEvaluator>;

  GridEvaluator eval(ds1.GetCoordinateSystem(),
                     ds1.GetCellSet(0),
                     fieldArray1,
                     0,
                     ds2.GetCoordinateSystem(),
                     ds2.GetCellSet(0),
                     fieldArray2,
                     10.0);

  Integrator integrator(eval, stepSize);

  // This example does not work on scale, works on basic 11 particles
  // so the results are more tractible
  std::vector<vtkm::Vec<FieldType, 3>> seeds;
  FieldType x = 0, y = 5, z = 0;
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
    particleAdvection.Run(integrator, seedArray, numSteps);
  }
  else
  {
    vtkm::worklet::Streamline streamline;
    vtkm::worklet::StreamlineResult res = streamline.Run(integrator, seedArray, numSteps);
    vtkm::cont::DataSet outData;
    vtkm::cont::CoordinateSystem outputCoords("coordinates", res.positions);
    outData.AddCellSet(res.polyLines);
    outData.AddCoordinateSystem(outputCoords);
    renderAndWriteDataSet(outData);
  }
}

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = vtkm::cont::Initialize(argc, argv, opts);

  std::cout << "TemporalAdvection Example" << std::endl;
  std::cout << "Parameters are [options] numSteps stepSize advectionType" << std::endl << std::endl;
  std::cout << "advectionType Particles=0 Streamlines=1" << std::endl;

  vtkm::Id numSteps;
  vtkm::Float32 stepSize;
  vtkm::Id advectionType;

  if (argc < 4)
  {
    std::cout << "Wrong number of parameters provided" << std::endl;
    exit(EXIT_FAILURE);
  }

  numSteps = atoi(argv[1]);
  stepSize = static_cast<vtkm::Float32>(atof(argv[2]));
  advectionType = atoi(argv[3]);

  RunTest(numSteps, stepSize, advectionType);

  return 0;
}
