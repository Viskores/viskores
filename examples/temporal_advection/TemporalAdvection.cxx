//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <cstdlib>
#include <string>
#include <vector>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/filter/Pathline.h>

#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = vtkm::cont::Initialize(argc, argv, opts);

  std::cout << "Temporal Advection Example" << std::endl;
  std::cout << "Parameters are [options] <dataset slice 1> <time 1> "
            << "<dataset slice 2> <time 2> <num steps> <step size> <output dataset>" << std::endl;

  if (argc < 7)
  {
    std::cout << "Wrong number of parameters provided" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string datasetName1, datasetName2, outputName;
  vtkm::FloatDefault time1, time2;

  vtkm::Id numSteps;
  vtkm::Float32 stepSize;

  datasetName1 = std::string(argv[1]);
  time1 = atof(argv[2]);
  datasetName2 = std::string(argv[3]);
  time2 = atof(argv[4]);
  numSteps = atoi(argv[5]);
  stepSize = static_cast<vtkm::Float32>(atof(argv[6]));
  outputName = std::string(argv[7]);

  vtkm::io::reader::VTKDataSetReader reader1(datasetName1);
  vtkm::cont::DataSet ds1 = reader1.ReadDataSet();

  vtkm::io::reader::VTKDataSetReader reader2(datasetName2);
  vtkm::cont::DataSet ds2 = reader2.ReadDataSet();

  // Use the coordinate system as seeds for performing advection
  vtkm::cont::ArrayHandle<vtkm::Vec3f> seeds;
  vtkm::cont::ArrayCopy(ds1.GetCoordinateSystem().GetData(), seeds);

  // Instantiate the filter by providing necessary parameters.
  // Necessary parameters are :
  vtkm::filter::Pathline pathlineFilter;
  // 1. The current and next time slice. The current time slice is passed
  //    through the parameter to the Execute method.
  pathlineFilter.SetNextDataSet(ds2);
  // 2. The current and next times, these times will be used to interpolate
  //    the velocities for particle positions in space and time.
  pathlineFilter.SetCurrentTime(time1);
  pathlineFilter.SetNextTime(time2);
  // 3. Maximum number of steps the particle is allowed to take until termination.
  pathlineFilter.SetNumberOfSteps(numSteps);
  // 4. Length for each step.
  pathlineFilter.SetStepSize(stepSize);
  // 5. Seeds for advection.
  pathlineFilter.SetSeeds(seeds);

  vtkm::cont::DataSet output = pathlineFilter.Execute(ds1);

  // The way to verify if the code produces correct streamlines
  // is to do a visual test by using VisIt/ParaView to visualize
  // the file written by this method.
  vtkm::io::writer::VTKDataSetWriter writer("pathlines.vtk");
  writer.WriteDataSet(output);
  return 0;
}
