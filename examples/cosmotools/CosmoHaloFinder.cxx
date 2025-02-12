//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Initialize.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/worklet/CosmoTools.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static const viskores::cont::LogLevel CosmoLogLevel = viskores::cont::LogLevel::UserFirst;

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

  for (viskores::Id p = 0; p < nParticles; p++)
  {
    inFile >> xLocation[p] >> yLocation[p] >> zLocation[p];
  }

  viskores::cont::ArrayHandle<viskores::Float32> xLocArray =
    viskores::cont::make_ArrayHandleMove<viskores::Float32>(xLocation, nParticles);
  viskores::cont::ArrayHandle<viskores::Float32> yLocArray =
    viskores::cont::make_ArrayHandleMove<viskores::Float32>(yLocation, nParticles);
  viskores::cont::ArrayHandle<viskores::Float32> zLocArray =
    viskores::cont::make_ArrayHandleMove<viskores::Float32>(zLocation, nParticles);

  // Output halo id, mbp id and min potential per particle
  viskores::cont::ArrayHandle<viskores::Id> resultHaloId;
  viskores::cont::ArrayHandle<viskores::Id> resultMBP;
  viskores::cont::ArrayHandle<viskores::Float32> resultPot;

  // Create the worklet and run it
  viskores::Id minHaloSize = 20;
  viskores::Float32 linkingLength = 0.2f;
  viskores::Float32 particleMass = 1.08413e+09f;

  {
    VISKORES_LOG_SCOPE(CosmoLogLevel, "Executing HaloFinder");

    viskores::worklet::CosmoTools cosmoTools;
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
  viskores::cont::SetLogLevelName(CosmoLogLevel, "Cosmo");
  viskores::cont::SetStderrLogLevel(CosmoLogLevel);

  auto opts = viskores::cont::InitializeOptions::DefaultAnyDevice;
  viskores::cont::InitializeResult config = viskores::cont::Initialize(argc, argv, opts);

  if (argc < 2)
  {
    std::cout << "Usage: " << std::endl << "$ " << argv[0] << " <input_file>" << std::endl;
    std::cout << config.Usage << std::endl;
    return 1;
  }

#ifndef VISKORES_ENABLE_LOGGING
  std::cout << "Warning: turn on Viskores_ENABLE_LOGGING CMake option to turn on timing." << std::endl;
#endif

  TestCosmoHaloFinder(argv[1]);

  return 0;
}
