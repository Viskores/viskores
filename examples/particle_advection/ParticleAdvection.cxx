//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/Particle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Initialize.h>
#include <viskores/filter/flow/Streamline.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

// Example computing streamlines.
// An example vector field is available in the viskores data directory: rotate-vectors.vtk
// Example usage:
//   this will advect 200 particles 50 steps using a step size of 0.05
//
// Particle_Advection <path-to-data-dir>/rotate-vectors.vtk rotate 200 50 0.05 output.vtk
//

int main(int argc, char** argv)
{
  auto opts = viskores::cont::InitializeOptions::DefaultAnyDevice;
  auto config = viskores::cont::Initialize(argc, argv, opts);

  if (argc < 7)
  {
    std::cerr << "Usage: " << argv[0]
              << " dataFile varName numSeeds numSteps stepSize outputFile [options]" << std::endl;
    std::cerr << "where options are: " << std::endl << config.Usage << std::endl;
    return -1;
  }

  std::string dataFile = argv[1];
  std::string varName = argv[2];
  viskores::Id numSeeds = std::stoi(argv[3]);
  viskores::Id numSteps = std::stoi(argv[4]);
  viskores::FloatDefault stepSize = std::stof(argv[5]);
  std::string outputFile = argv[6];

  viskores::cont::DataSet ds;

  if (dataFile.find(".vtk") != std::string::npos)
  {
    viskores::io::VTKDataSetReader rdr(dataFile);
    ds = rdr.ReadDataSet();
  }
  else
  {
    std::cerr << "Unsupported data file: " << dataFile << std::endl;
    return -1;
  }

  //create seeds randomly placed withing the bounding box of the data.
  viskores::Bounds bounds = ds.GetCoordinateSystem().GetBounds();
  std::vector<viskores::Particle> seeds;

  for (viskores::Id i = 0; i < numSeeds; i++)
  {
    viskores::Particle p;
    viskores::FloatDefault rx = (viskores::FloatDefault)rand() / (viskores::FloatDefault)RAND_MAX;
    viskores::FloatDefault ry = (viskores::FloatDefault)rand() / (viskores::FloatDefault)RAND_MAX;
    viskores::FloatDefault rz = (viskores::FloatDefault)rand() / (viskores::FloatDefault)RAND_MAX;
    p.SetPosition({ static_cast<viskores::FloatDefault>(bounds.X.Min + rx * bounds.X.Length()),
                    static_cast<viskores::FloatDefault>(bounds.Y.Min + ry * bounds.Y.Length()),
                    static_cast<viskores::FloatDefault>(bounds.Z.Min + rz * bounds.Z.Length()) });
    p.SetID(i);
    seeds.push_back(p);
  }
  auto seedArray = viskores::cont::make_ArrayHandle(seeds, viskores::CopyFlag::Off);

  //compute streamlines
  viskores::filter::flow::Streamline streamline;

  streamline.SetStepSize(stepSize);
  streamline.SetNumberOfSteps(numSteps);
  streamline.SetSeeds(seedArray);

  streamline.SetActiveField(varName);
  auto output = streamline.Execute(ds);

  viskores::io::VTKDataSetWriter wrt(outputFile);
  wrt.WriteDataSet(output);

  return 0;
}
