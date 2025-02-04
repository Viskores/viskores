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

void TestCosmoCenterFinder(const char* fileName)
{
  std::cout << std::endl
            << "Testing Cosmology MBP Center Finder Filter on one halo " << fileName << std::endl;

  // Open the file for reading
  std::ifstream inFile(fileName);
  if (inFile.fail())
  {
    std::cout << "File does not exist " << fileName << std::endl;
    return;
  }

  // Read in number of particles and locations
  viskores::Id nParticles;
  inFile >> nParticles;

  float* xLocation = new float[static_cast<std::size_t>(nParticles)];
  float* yLocation = new float[static_cast<std::size_t>(nParticles)];
  float* zLocation = new float[static_cast<std::size_t>(nParticles)];
  std::cout << "Running MBP on " << nParticles << std::endl;

  for (viskores::Id p = 0; p < nParticles; p++)
  {
    inFile >> xLocation[p] >> yLocation[p] >> zLocation[p];
  }

  viskores::cont::ArrayHandle<viskores::Float32> xLocArray =
    viskores::cont::make_ArrayHandle<viskores::Float32>(xLocation, nParticles, viskores::CopyFlag::Off);
  viskores::cont::ArrayHandle<viskores::Float32> yLocArray =
    viskores::cont::make_ArrayHandle<viskores::Float32>(yLocation, nParticles, viskores::CopyFlag::Off);
  viskores::cont::ArrayHandle<viskores::Float32> zLocArray =
    viskores::cont::make_ArrayHandle<viskores::Float32>(zLocation, nParticles, viskores::CopyFlag::Off);

  // Output MBP particleId pairs array
  viskores::Pair<viskores::Id, viskores::Float32> nxnResult;
  viskores::Pair<viskores::Id, viskores::Float32> mxnResult;

  const viskores::Float32 particleMass = 1.08413e+09f;
  viskores::worklet::CosmoTools cosmoTools;

  {
    VISKORES_LOG_SCOPE(CosmoLogLevel, "Executing NxN");

    cosmoTools.RunMBPCenterFinderNxN(
      xLocArray, yLocArray, zLocArray, nParticles, particleMass, nxnResult);

    VISKORES_LOG_S(CosmoLogLevel,
               "NxN MPB = " << nxnResult.first << "  potential = " << nxnResult.second);
  }

  {
    VISKORES_LOG_SCOPE(CosmoLogLevel, "Executing MxN");
    cosmoTools.RunMBPCenterFinderMxN(
      xLocArray, yLocArray, zLocArray, nParticles, particleMass, mxnResult);

    VISKORES_LOG_S(CosmoLogLevel,
               "MxN MPB = " << mxnResult.first << "  potential = " << mxnResult.second);
  }

  if (nxnResult.first == mxnResult.first)
    std::cout << "FOUND CORRECT PARTICLE " << mxnResult.first << " with potential "
              << nxnResult.second << std::endl;
  else
    std::cout << "ERROR DID NOT FIND SAME PARTICLE" << std::endl;

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
// CosmoCenterFinder data.cosmotools
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

  TestCosmoCenterFinder(argv[1]);

  return 0;
}
