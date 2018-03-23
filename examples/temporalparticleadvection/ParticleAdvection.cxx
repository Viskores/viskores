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

#include <cstdlib>
#include <vector>

const vtkm::Id SPARSE = 0;
const vtkm::Id DENSE = 1;
const vtkm::Id MEDIUM = 2;

template <typename T>
static vtkm::Range subRange(vtkm::Range& range, T a, T b)
{
  vtkm::Float32 arg1, arg2, len;
  arg1 = static_cast<vtkm::Float32>(a);
  arg2 = static_cast<vtkm::Float32>(b);
  len = static_cast<vtkm::Float32>(range.Length());
  return vtkm::Range(range.Min + arg1 * len, range.Min + arg2 * len);
}

template <typename T>
void ignore(T&&)
{
}

void RunTest(const std::string& fname,
             vtkm::Id numSeeds,
             vtkm::Id numSteps,
             vtkm::Float32 stepSize,
             vtkm::Id seeding)
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;

  using FieldType = vtkm::Float32;
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>;
  using FieldPortalConstType =
    typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;

  vtkm::io::reader::BOVDataSetReader reader(fname);
  vtkm::cont::DataSet ds = reader.ReadDataSet();

  using GridEvaluator =
    vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldPortalConstType,
                                                            FieldType,
                                                            DeviceAdapter>;
  using Integrator = vtkm::worklet::particleadvection::EulerIntegrator<GridEvaluator, FieldType>;

  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> fieldArray;
  ds.GetField(0).GetData().CopyTo(fieldArray);

  //GridEvaluator eval(ds.GetCoordinateSystem(), ds.GetCellSet(0), fieldArray);
  GridEvaluator eval(ds.GetCoordinateSystem(),
                     ds.GetCellSet(0),
                     fieldArray,
                     0,
                     ds.GetCoordinateSystem(),
                     ds.GetCellSet(0),
                     fieldArray,
                     1.0);
  Integrator integrator(eval, stepSize);

  std::vector<vtkm::Vec<FieldType, 3>> seeds;
  srand(314);

  vtkm::Bounds bounds = ds.GetCoordinateSystem().GetBounds();
  if (seeding == SPARSE)
    bounds = ds.GetCoordinateSystem().GetBounds();
  else if (seeding == DENSE)
  {
    if (fname.find("astro") != std::string::npos)
    {
      bounds.X = subRange(bounds.X, .1, .15);
      bounds.Y = subRange(bounds.Y, .1, .15);
      bounds.Z = subRange(bounds.Z, .1, .15);
    }
    else if (fname.find("fusion") != std::string::npos)
    {
      bounds.X = subRange(bounds.X, .8, .85);
      bounds.Y = subRange(bounds.Y, .55, .60);
      bounds.Z = subRange(bounds.Z, .55, .60);
    }
    else if (fname.find("fishtank") != std::string::npos)
    {
      bounds.X = subRange(bounds.X, .1, .15);
      bounds.Y = subRange(bounds.Y, .1, .15);
      bounds.Z = subRange(bounds.Z, .55, .60);
    }
  }
  else if (seeding == MEDIUM)
  {
    if (fname.find("astro") != std::string::npos)
    {
      bounds.X = subRange(bounds.X, .4, .6);
      bounds.Y = subRange(bounds.Y, .4, .6);
      bounds.Z = subRange(bounds.Z, .4, .6);
    }
    else if (fname.find("fusion") != std::string::npos)
    {
      bounds.X = subRange(bounds.X, .01, .99);
      bounds.Y = subRange(bounds.Y, .01, .99);
      bounds.Z = subRange(bounds.Z, .45, .55);
    }
    else if (fname.find("fishtank") != std::string::npos)
    {
      bounds.X = subRange(bounds.X, .4, .6);
      bounds.Y = subRange(bounds.Y, .4, .6);
      bounds.Z = subRange(bounds.Z, .4, .6);
    }
  }

  for (int i = 0; i < numSeeds; i++)
  {
    vtkm::Vec<FieldType, 3> point;
    vtkm::Float32 rx = (vtkm::Float32)rand() / (vtkm::Float32)RAND_MAX;
    vtkm::Float32 ry = (vtkm::Float32)rand() / (vtkm::Float32)RAND_MAX;
    vtkm::Float32 rz = (vtkm::Float32)rand() / (vtkm::Float32)RAND_MAX;
    point[0] = static_cast<FieldType>(bounds.X.Min + rx * bounds.X.Length());
    point[1] = static_cast<FieldType>(bounds.Y.Min + ry * bounds.Y.Length());
    point[2] = static_cast<FieldType>(bounds.Z.Min + rz * bounds.Z.Length());
    seeds.push_back(point);
  }

  /*#ifdef __BUILDING_TBB_VERSION__
  int nT = tbb::task_scheduler_init::default_num_threads();
  if (numThreads != -1)
    nT = (int)numThreads;
  //make sure the task_scheduler_init object is in scope when running sth w/ TBB
  tbb::task_scheduler_init init(nT);
#else
  ignore(numThreads);
#endif
*/

  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> seedArray;
  seedArray = vtkm::cont::make_ArrayHandle(seeds);

  //  if (advectType == 0)
  //  {
  vtkm::worklet::ParticleAdvection particleAdvection;
  particleAdvection.Run(integrator, seedArray, numSteps, DeviceAdapter());
  //  }
  /*else
  {
    vtkm::worklet::Streamline streamline;
    streamline.Run(integrator, seedArray, numSteps, DeviceAdapter());
  }*/
}

int main(int argc, char** argv)
{
  vtkm::Id numSeeds, numSteps;
  vtkm::Float32 stepSize;
  std::string dataFile;
  vtkm::Id seeding = SPARSE;

  if (argc < 5)
  {
    std::cout << "Wrong number of parameters provided" << std::endl;
    exit(EXIT_FAILURE);
  }

  dataFile = std::string(argv[1]);
  numSeeds = atoi(argv[2]);
  numSteps = atoi(argv[3]);
  stepSize = atof(argv[4]);

  RunTest(dataFile, numSeeds, numSteps, stepSize, seeding);
  return 0;
}
