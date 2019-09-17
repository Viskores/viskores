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
#include <vtkm/filter/Streamline.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

//timers
#include <chrono>


// Example computing streamlines.
// An example vector field is available in the vtk-m data directory: magField.vtk
// Example usage:
//   this will advect 200 particles 50 steps using a step size of 0.01
//
// Particle_Advection <path-to-data-dir>/magField.vtk vec 200 50 0.01 output.vtk
//

int main(int argc, char** argv)
{
  if (argc != 7 && argc != 8)
  {
    std::cerr << "Usage: " << argv[0]
              << " dataFile varName numSeeds numSteps stepSize outputFile <device>" << std::endl;
    return -1;
  }
  std::string device = "serial";
  if (argc == 8)
    device = argv[7];

  if (device == "serial" &&
      vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(vtkm::cont::DeviceAdapterTagSerial{}))
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagSerial{});
  else if (device == "tbb" &&
           vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(vtkm::cont::DeviceAdapterTagTBB{}))
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagTBB{});
  else if (device == "openmp" &&
           vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(vtkm::cont::DeviceAdapterTagOpenMP{}))
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagOpenMP{});
  else if (device == "cuda" &&
           vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(vtkm::cont::DeviceAdapterTagCuda{}))
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagCuda{});
  else
  {
    std::cerr << " Unknown device: " << device << ". Valid options are: serial, tbb, openmp, cuda"
              << std::endl;
    return -1;
  }

  std::cout << "Using device= " << device << std::endl;

  std::string dataFile = argv[1];
  std::string varName = argv[2];
  vtkm::Id numSeeds = std::stoi(argv[3]);
  vtkm::Id numSteps = std::stoi(argv[4]);
  vtkm::FloatDefault stepSize = std::stof(argv[5]);
  std::string outputFile = argv[6];

  vtkm::cont::DataSet ds;

  std::cout << "Reading data..." << std::endl;
  if (dataFile.find(".vtk") != std::string::npos)
  {
    vtkm::io::reader::VTKDataSetReader rdr(dataFile);
    ds = rdr.ReadDataSet();
  }
  else
  {
    std::cerr << "Unsupported data file: " << dataFile << std::endl;
    return -1;
  }

  //create seeds randomly placed withing the bounding box of the data.
  vtkm::Bounds bounds = ds.GetCoordinateSystem().GetBounds();
  std::vector<vtkm::Vec3f> seeds;
  std::vector<vtkm::Particle> particles;

  for (int i = 0; i < numSeeds; i++)
  {
    vtkm::Vec3f p;
    vtkm::FloatDefault rx = (vtkm::FloatDefault)rand() / (vtkm::FloatDefault)RAND_MAX;
    vtkm::FloatDefault ry = (vtkm::FloatDefault)rand() / (vtkm::FloatDefault)RAND_MAX;
    vtkm::FloatDefault rz = (vtkm::FloatDefault)rand() / (vtkm::FloatDefault)RAND_MAX;
    p[0] = static_cast<vtkm::FloatDefault>(bounds.X.Min + rx * bounds.X.Length());
    p[1] = static_cast<vtkm::FloatDefault>(bounds.Y.Min + ry * bounds.Y.Length());
    p[2] = static_cast<vtkm::FloatDefault>(bounds.Z.Min + rz * bounds.Z.Length());
    seeds.push_back(p);

    vtkm::Particle pa;
    pa.ID = i;
    pa.Status = vtkm::ParticleStatus::SUCCESS;
    pa.Pos = p;
    pa.NumSteps = 0;
    particles.push_back(pa);
  }

  auto seedArrayPts = vtkm::cont::make_ArrayHandle(seeds);
  auto seedArrayPar = vtkm::cont::make_ArrayHandle(particles);

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;

  auto vec = ds.GetField(varName).GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();
  GridEvalType eval(ds.GetCoordinateSystem(), ds.GetCellSet(), vec);
  RK4Type rk4(eval, stepSize);


  vtkm::worklet::ParticleAdvection paPTS, paPART;
  std::chrono::duration<double> dT;

  std::cout << "Advect particles." << std::endl;
  if (1)
  {
    auto s = std::chrono::system_clock::now();
    auto res0 = paPTS.Run(rk4, seedArrayPts, numSteps);
    auto e = std::chrono::system_clock::now();
    dT = e - s;
    std::cout << "Time= " << dT.count() << std::endl;
  }

  if (1)
  {
    auto s = std::chrono::system_clock::now();
    auto res1 = paPART.Run(rk4, seedArrayPar, numSteps);
    auto e = std::chrono::system_clock::now();
    dT = e - s;
    std::cout << "AOS_Time= " << dT.count() << std::endl;
  }

  /*
  //compute streamlines
  vtkm::filter::Streamline streamline;

  streamline.SetStepSize(stepSize);
  streamline.SetNumberOfSteps(numSteps);
  streamline.SetSeeds(seedArray);

  streamline.SetActiveField(varName);
  auto output = streamline.Execute(ds);

  vtkm::io::writer::VTKDataSetWriter wrt(outputFile);
  wrt.WriteDataSet(output);
  */

  return 0;
}

/*
runs on whoopingcough: CPU
./examples/particle_advection/Particle_Advection ../fish128.vtk grad 1000000 1000 0.001 x.out
Time= 52.794
AOS_Time= 52.347


*/
