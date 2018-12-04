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
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

#include "vtkm/Math.h"
#include "vtkm/cont/ArrayHandle.h"
#include "vtkm/cont/DataSetBuilderUniform.h"

#include "vtkm/filter/FilterDataSet.h"

#include "vtkm/cont/TryExecute.h"
#include "vtkm/cont/cuda/DeviceAdapterCuda.h"
#include "vtkm/cont/serial/DeviceAdapterSerial.h"
#include "vtkm/cont/tbb/DeviceAdapterTBB.h"

#include "vtkm/filter/OscillatorSource.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

//Suppress warnings about glut being deprecated on OSX
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

//This is the list of devices to compile in support for. The order of the
//devices determines the runtime preference.
struct DevicesToTry : vtkm::ListTagBase<vtkm::cont::DeviceAdapterTagCuda,
                                        vtkm::cont::DeviceAdapterTagTBB,
                                        vtkm::cont::DeviceAdapterTagSerial>
{
};

// ----------------------------------------------------------------------------

struct OscillatorPolicy : public vtkm::filter::PolicyBase<OscillatorPolicy>
{
  using DeviceAdapterList = DevicesToTry;
};

// trim() from http://stackoverflow.com/a/217605/44738
static inline std::string& ltrim(std::string& s)
{
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}
static inline std::string& rtrim(std::string& s)
{
  s.erase(
    std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
    s.end());
  return s;
}
static inline std::string& trim(std::string& s)
{
  return ltrim(rtrim(s));
}

// ----------------------------------------------------------------------------

void read_oscillators(std::string filePath, vtkm::filter::OscillatorSource& filter)
{
  std::ifstream in(filePath);
  if (!in)
    throw std::runtime_error("Unable to open " + filePath);
  std::string line;
  while (std::getline(in, line))
  {
    line = trim(line);
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream iss(line);

    std::string stype;
    iss >> stype;

    int x, y, z;
    iss >> x >> y >> z;

    float r, omega0, zeta = 0.0f;
    iss >> r >> omega0;

    if (stype == "damped")
    {
      iss >> zeta;
    }

    if (stype == "damped")
    {
      filter.AddDamped(x, y, z, r, omega0, zeta);
    }
    else if (stype == "decaying")
    {
      filter.AddDecaying(x, y, z, r, omega0, zeta);
    }
    else if (stype == "periodic")
    {
      filter.AddPeriodic(x, y, z, r, omega0, zeta);
    }
  }
}

// ----------------------------------------------------------------------------
// ArcticViewer helper
// ----------------------------------------------------------------------------

void writeData(std::string& basePath, int timestep, int iSize, int jSize, int kSize, double* values)
{
  int size = iSize * jSize * kSize;
  std::ostringstream timeValues;
  for (int t = 0; t <= timestep; t++)
  {
    if (t > 0)
    {
      timeValues << ", ";
    }
    timeValues << "\"" << t << "\"";
  }

  // Root metadata file
  std::ostringstream metaFilePath;
  metaFilePath << basePath.c_str() << "/index.json";
  std::ofstream metaFilePathPointer(metaFilePath.str().c_str(), std::ios::out);
  if (metaFilePathPointer.fail())
  {
    std::cout << "Unable to open file: " << metaFilePath.str().c_str() << std::endl;
  }
  else
  {
    metaFilePathPointer << "{\n"
                        << "  \"metadata\": {\"backgroundColor\": \"#000000\"},\n"
                        << "  \"type\": [\"tonic-query-data-model\", \"vtk-volume\"],\n"
                        << "  \"arguments_order\": [\"time\"],\n"
                        << "  \"arguments\": {\n"
                        << "    \"time\": {\n"
                        << "       \"loop\": \"modulo\",\n"
                        << "       \"ui\": \"slider\",\n"
                        << "       \"values\": [" << timeValues.str().c_str() << "]\n"
                        << "    }\n"
                        << "  },\n"
                        << "  \"data\": [{\n"
                        << "    \"pattern\": \"{time}.json\",\n"
                        << "    \"rootFile\": true,\n"
                        << "    \"name\": \"scene\",\n"
                        << "    \"type\": \"json\"\n"
                        << "  }]\n"
                        << "}\n";
    metaFilePathPointer.flush();
    metaFilePathPointer.close();
  }

  // Current data file
  std::ostringstream dataFilePath;
  dataFilePath << basePath.c_str() << "/" << timestep << ".data";
  std::ofstream dataFilePathPointer(dataFilePath.str().c_str(), std::ios::out | std::ios::binary);
  if (dataFilePathPointer.fail())
  {
    std::cout << "Unable to open file: " << dataFilePath.str().c_str() << std::endl;
  }
  else
  {
    int stackSize = size * 8;
    dataFilePathPointer.write((char*)values, stackSize);
    dataFilePathPointer.flush();
    dataFilePathPointer.close();
  }

  // Current dataset meta file
  std::ostringstream dataMetaFilePath;
  dataMetaFilePath << basePath.c_str() << "/" << timestep << ".json";
  std::ofstream dataMetaFilePathPointer(dataMetaFilePath.str().c_str(), std::ios::out);
  if (dataMetaFilePathPointer.fail())
  {
    std::cout << "Unable to open file: " << dataMetaFilePath.str().c_str() << std::endl;
  }
  else
  {
    dataMetaFilePathPointer << "{\n"
                            << "  \"origin\": [0,0,0],\n"
                            << "  \"spacing\": [1,1,1],\n"
                            << "  \"extent\": [\n"
                            << "       0, " << (iSize - 1) << ",\n"
                            << "       0, " << (jSize - 1) << ",\n"
                            << "       0, " << (kSize - 1) << "],\n"
                            << "  \"vtkClass\": \"vtkImageData\",\n"
                            << "  \"pointData\": {\n"
                            << "      \"vtkClass\": \"vtkDataSetAttributes\",\n"
                            << "      \"arrays\": [{\n"
                            << "          \"data\": {\n"
                            << "              \"numberOfComponents\": 1,\n"
                            << "              \"name\": \"oscillation\",\n"
                            << "              \"vtkClass\": \"vtkDataArray\",\n"
                            << "              \"dataType\": \"Float64Array\",\n"
                            << "              \"ref\": {\n"
                            << "                  \"registration\": \"setScalars\",\n"
                            << "                  \"encode\": \"LittleEndian\",\n"
                            << "                  \"basepath\": \"\",\n"
                            << "                  \"id\": \"" << timestep << ".data\"\n"
                            << "              },\n"
                            << "              \"size\": " << size << std::endl
                            << "          }\n"
                            << "      }]\n"
                            << "  }\n"
                            << "}\n";
    dataMetaFilePathPointer.flush();
    dataMetaFilePathPointer.close();
  }
}

// ----------------------------------------------------------------------------

void printUsage()
{
  std::cout << "Usage: Oscillator [options]\n"
            << std::endl
            << "Options:\n"
            << std::endl
            << "  -s, --shape POINT     domain shape [default: 64x64x64]" << std::endl
            << "  -t, --dt FLOAT        time step [default: 0.01]" << std::endl
            << "  -f, --config STRING   oscillator file (required)" << std::endl
            << "      --t-end FLOAT     end time [default: 10]" << std::endl
            << "  -o, --output STRING   directory to output data\n"
            << std::endl;
}

// ----------------------------------------------------------------------------

int main(int argc, char** argv)
{
  std::string oscillatorConfigFile = "";
  std::string outputDirectory = "";
  int sizeX = 64;
  int sizeY = 64;
  int sizeZ = 64;
  float startTime = 0.0f;
  float endTime = 10.0f;
  float deltaTime = 0.01f;
  float currentTime = startTime;
  bool generateOutput = false;

  // Process args
  int nbOptions = argc - 1;
  for (int i = 1; i < nbOptions; i += 2)
  {
    // Config (REQUIRED)
    if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--config"))
    {
      oscillatorConfigFile = argv[i + 1];
    }
    // Output
    if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
    {
      generateOutput = true;
      outputDirectory = argv[i + 1];
    }

    // Shape
    if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--shape"))
    {
      std::string shape = argv[i + 1];
      size_t found = shape.find("x");
      sizeX = sizeY = sizeZ = std::stoi(shape);
      if (found != std::string::npos)
      {
        shape = shape.substr(found + 1);
        sizeY = sizeZ = std::stoi(shape);
        found = shape.find("x");
        if (found != std::string::npos)
        {
          shape = shape.substr(found + 1);
          sizeZ = std::stoi(shape);
          found = shape.find("x");
        }
      }
    }

    // Time
    if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--dt"))
    {
      deltaTime = float(std::atof(argv[i + 1]));
    }

    // End-Time
    if (!strcmp(argv[i], "--t-end"))
    {
      endTime = float(std::atof(argv[i + 1]));
    }
  }

  if (oscillatorConfigFile.size() < 2)
  {
    printUsage();
    return 0;
  }

  std::cout << "\n=========== configuration ============" << std::endl;
  std::cout << " oscillator config: " << oscillatorConfigFile.c_str() << std::endl;
  std::cout << " mesh size: " << sizeX << "x" << sizeY << "x" << sizeZ << std::endl;
  std::cout << " time handling:" << std::endl;
  std::cout << "  - dt: " << deltaTime << std::endl;
  std::cout << "  - end: " << endTime << std::endl;
  std::cout << "=======================================\n" << std::endl;

  vtkm::cont::DataSetBuilderUniform builder;
  vtkm::cont::DataSet dataset = builder.Create(vtkm::Id3(sizeX, sizeY, sizeZ));

  vtkm::filter::OscillatorSource filter;
  read_oscillators(oscillatorConfigFile, filter);

  std::cout << "=========== start computation ============" << std::endl;
  int count = 0;
  while (currentTime < endTime)
  {
    filter.SetTime(currentTime);
    vtkm::cont::DataSet rdata = filter.Execute(dataset, OscillatorPolicy());
    if (generateOutput)
    {
      vtkm::cont::ArrayHandle<vtkm::Float64> tmp;
      rdata.GetField("oscillation", vtkm::cont::Field::Association::POINTS).GetData().CopyTo(tmp);
      double* values = tmp.GetStorage().GetArray();
      writeData(outputDirectory, count++, sizeX, sizeY, sizeZ, values);
    }

    std::cout << "Compute time " << currentTime << std::endl;
    currentTime += deltaTime;
  }
  std::cout << "=========== computation done ============" << std::endl;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif
