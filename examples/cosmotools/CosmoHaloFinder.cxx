//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Initialize.h>

#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

#include <vtkm/worklet/CosmoTools.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static const vtkm::cont::LogLevel CosmoLogLevel = vtkm::cont::LogLevel::UserFirst;

void TestCosmoHaloFinder(const char* fileName)
{
  std::cout << std::endl
            << "Testing Cosmology Halo Finder and MBP Center Finder " << fileName << std::endl;

  // Open the file for reading
  std::ifstream inFile(fileName);
  if (inFile.fail())
  {
    std::cout << "File does not exist " << fileName << std::endl;
    return;
  }

  // Read in number of particles and locations
  int nParticles;
  inFile >> nParticles;
  std::size_t size = static_cast<std::size_t>(nParticles);

  float* xLocation = new float[size];
  float* yLocation = new float[size];
  float* zLocation = new float[size];
  std::cout << "Running Halo Finder on " << nParticles << std::endl;

  for (vtkm::Id p = 0; p < nParticles; p++)
  {
    inFile >> xLocation[p] >> yLocation[p] >> zLocation[p];
  }

  vtkm::cont::ArrayHandle<vtkm::Float32> xLocArray =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(xLocation, nParticles);
  vtkm::cont::ArrayHandle<vtkm::Float32> yLocArray =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(yLocation, nParticles);
  vtkm::cont::ArrayHandle<vtkm::Float32> zLocArray =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(zLocation, nParticles);

  // Output halo id, mbp id and min potential per particle
  vtkm::cont::ArrayHandle<vtkm::Id> resultHaloId;
  vtkm::cont::ArrayHandle<vtkm::Id> resultMBP;
  vtkm::cont::ArrayHandle<vtkm::Float32> resultPot;

  // Create the worklet and run it
  vtkm::Id minHaloSize = 20;
  vtkm::Float32 linkingLength = 0.2f;
  vtkm::Float32 particleMass = 1.08413e+09f;

  {
    VTKM_LOG_SCOPE(CosmoLogLevel, "Executing HaloFinder");

    vtkm::worklet::CosmoTools cosmoTools;
    cosmoTools.RunHaloFinder(xLocArray,
                             yLocArray,
                             zLocArray,
                             nParticles,
                             particleMass,
                             minHaloSize,
                             linkingLength,
                             resultHaloId,
                             resultMBP,
                             resultPot);
  }

  xLocArray.ReleaseResources();
  yLocArray.ReleaseResources();
  zLocArray.ReleaseResources();

  delete[] xLocation;
  delete[] yLocation;
  delete[] zLocation;
}

/////////////////////////////////////////////////////////////////////
//
// Form of the input file in ASCII
// Line 1: number of particles in the file
// Line 2+: (float) xLoc (float) yLoc (float) zLoc
//
// CosmoHaloFinder data.cosmotools
//
/////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  vtkm::cont::SetLogLevelName(CosmoLogLevel, "Cosmo");
  vtkm::cont::SetStderrLogLevel(CosmoLogLevel);

  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  if (argc < 2)
  {
    std::cout << "Usage: " << std::endl << "$ " << argv[0] << " <input_file>" << std::endl;
    std::cout << config.Usage << std::endl;
    return 1;
  }

#ifndef VTKM_ENABLE_LOGGING
  std::cout << "Warning: turn on VTKm_ENABLE_LOGGING CMake option to turn on timing." << std::endl;
#endif

  TestCosmoHaloFinder(argv[1]);

  return 0;
}
