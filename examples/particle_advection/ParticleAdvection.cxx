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
#include <vtkm/filter/Streamline.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

// Example computing streamlines.
// An example vector field is available in the vtk-m data directory: magField.vtk
// Example usage:
//   this will advect 200 particles 50 steps using a step size of 0.01
//
// Particle_Advection <path-to-data-dir>/magField.vtk vec 200 50 0.01 output.vtk
//

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = vtkm::cont::Initialize(argc, argv, opts);

  if (argc < 8)
  {
    std::cerr << "Usage: " << argv[0]
              << "dataFile varName numSeeds numSteps stepSize outputFile [options]" << std::endl;
    std::cerr << "where options are: " << std::endl << config.Usage << std::endl;
    return -1;
  }

  std::string dataFile = argv[1];
  std::string varName = argv[2];
  vtkm::Id numSeeds = std::stoi(argv[3]);
  vtkm::Id numSteps = std::stoi(argv[4]);
  vtkm::FloatDefault stepSize = std::stof(argv[5]);
  std::string outputFile = argv[6];

  vtkm::cont::DataSet ds;

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
  std::vector<vtkm::Particle> seeds;

  for (vtkm::Id i = 0; i < numSeeds; i++)
  {
    vtkm::Particle p;
    vtkm::FloatDefault rx = (vtkm::FloatDefault)rand() / (vtkm::FloatDefault)RAND_MAX;
    vtkm::FloatDefault ry = (vtkm::FloatDefault)rand() / (vtkm::FloatDefault)RAND_MAX;
    vtkm::FloatDefault rz = (vtkm::FloatDefault)rand() / (vtkm::FloatDefault)RAND_MAX;
    p.Pos[0] = static_cast<vtkm::FloatDefault>(bounds.X.Min + rx * bounds.X.Length());
    p.Pos[1] = static_cast<vtkm::FloatDefault>(bounds.Y.Min + ry * bounds.Y.Length());
    p.Pos[2] = static_cast<vtkm::FloatDefault>(bounds.Z.Min + rz * bounds.Z.Length());
    p.ID = i;
    seeds.push_back(p);
  }
  auto seedArray = vtkm::cont::make_ArrayHandle(seeds);

  //compute streamlines
  vtkm::filter::Streamline streamline;

  streamline.SetStepSize(stepSize);
  streamline.SetNumberOfSteps(numSteps);
  streamline.SetSeeds(seedArray);

  streamline.SetActiveField(varName);
  auto output = streamline.Execute(ds);

  vtkm::io::writer::VTKDataSetWriter wrt(outputFile);
  wrt.WriteDataSet(output);

  return 0;
}
