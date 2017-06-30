//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/ParticleAdvectionFilters.h>

#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/cont/Timer.h>

#include <vector>
#include <chrono>
#include <string>

const vtkm::Id SPARSE=0;
const vtkm::Id DENSE=1;
const vtkm::Id MEDIUM=2;

static vtkm::Range
subRange(vtkm::Range &range, vtkm::Float32 a, vtkm::Float32 b)
{
    vtkm::Float32 len = static_cast<vtkm::Float32>(range.Length());
    return vtkm::Range(range.Min + a*len,
                       range.Min + b*len);
}

void RunTest(const std::string &fname,
             vtkm::Id numSeeds,
             vtkm::Id numSteps,
             vtkm::Float32 stepSize,
             vtkm::Id numThreads,
             vtkm::Id advectType,
             vtkm::Id stepsPerRound,
             vtkm::Id particlesPerRound,
             bool dumpOutput,
             vtkm::Id seeding)
{
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  typedef vtkm::Float32 FieldType;
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalConstType;
  
  vtkm::io::reader::VTKRectilinearGridReader rdr(fname.c_str());
  vtkm::cont::DataSet ds = rdr.ReadDataSet();

  vtkm::worklet::particleadvection::RectilinearGridEvaluate<FieldPortalConstType, DeviceAdapter, FieldType> eval(ds);

  typedef vtkm::worklet::particleadvection::RectilinearGridEvaluate<FieldPortalConstType, DeviceAdapter, FieldType> RGEvalType;
  typedef vtkm::worklet::particleadvection::RK4Integrator<RGEvalType,FieldType,FieldPortalConstType> RK4RGType;
  
  RK4RGType rk4(eval, stepSize);

  std::vector<vtkm::Vec<FieldType,3> > seeds;
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
      vtkm::Vec<FieldType, 3> p;
      vtkm::Float32 rx = (vtkm::Float32)rand()/(vtkm::Float32)RAND_MAX;
      vtkm::Float32 ry = (vtkm::Float32)rand()/(vtkm::Float32)RAND_MAX;
      vtkm::Float32 rz = (vtkm::Float32)rand()/(vtkm::Float32)RAND_MAX;
      p[0] = static_cast<FieldType>(bounds.X.Min + rx*bounds.X.Length());
      p[1] = static_cast<FieldType>(bounds.Y.Min + ry*bounds.Y.Length());
      p[2] = static_cast<FieldType>(bounds.Z.Min + rz*bounds.Z.Length());
      seeds.push_back(p);
  }

#ifdef __BUILDING_TBB_VERSION__
  int nT = tbb::task_scheduler_init::default_num_threads();
  if (numThreads != -1)
    nT = numThreads;  
  //make sure the task_scheduler_init object is in scope when running sth w/ TBB
  tbb::task_scheduler_init init(nT);
#endif

  //time only the actual run
  auto t0 = std::chrono::high_resolution_clock::now();
  if (advectType == 0)
  {
      vtkm::worklet::particleadvection::ParticleAdvectionFilter<RK4RGType,
                                                                FieldType,
                                                                DeviceAdapter> pa(rk4,seeds,ds,numSteps);
      pa.run(dumpOutput);
  }
  else
  {
      vtkm::worklet::particleadvection::StreamlineFilter<RK4RGType,
                                                         FieldType,
                                                         DeviceAdapter> sl(rk4,seeds,ds,numSteps,stepsPerRound,particlesPerRound);
      sl.run(dumpOutput);
  }
  auto t1 = std::chrono::high_resolution_clock::now() - t0;
  std::uint64_t runtime = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();
  std::cerr<<"Runtime = "<<runtime<<" ms "<<std::endl;
}

bool ParseArgs(int argc, char **argv,
               vtkm::Id &numSeeds, vtkm::Id &numSteps, vtkm::Float32 &stepSize,
               vtkm::Id &advectType, vtkm::Id &stepsPerRound, vtkm::Id &particlesPerRound,
               vtkm::Id &numThreads, std::string &dataFile,
               std::string &pgmType, bool &dumpOutput, vtkm::Id &seeding)
{
    numSeeds = 100;
    numSteps = 100;
    stepSize = 0.1f;
    advectType = 0;
    stepsPerRound = -1;
    particlesPerRound = -1;
    numThreads = -1;
    dataFile = "";
    pgmType = "UNKNOWN";
    dumpOutput = false;
    seeding = SPARSE;

    if (argc < 2)
    {
        std::cerr<<"Usage "<<argv[0]<<std::endl;
        std::cerr<<" -seeds #seeds"<<std::endl;
        std::cerr<<" -steps maxSteps"<<std::endl;
        std::cerr<<" -h stepSize"<<std::endl;
        std::cerr<<" -particle : particle push"<<std::endl;
        std::cerr<<" -streamline steps_per_round (-1 = 0 rounds): particle history"<<std::endl;
        std::cerr<<" -t #numThreads"<<std::endl;
        std::cerr<<" -file dataFile"<<std::endl;
        std::cerr<<" -dump : dump output points"<<std::endl;
        return false;
    }

    std::string pgm = argv[0];
    if (pgm.find("SERIAL") != std::string::npos)
        pgmType = "SER";
    else if (pgm.find("TBB") != std::string::npos)
        pgmType = "TBB";
    else if (pgm.find("CUDA") != std::string::npos)
        pgmType = "CUD";
    
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-seeds")
            numSeeds = static_cast<vtkm::Id>(atoi(argv[++i]));
        else if (arg == "-steps")
            numSteps = static_cast<vtkm::Id>(atoi(argv[++i]));
        else if (arg == "-h")
            stepSize = static_cast<vtkm::Float32>(atof(argv[++i]));
        else if (arg == "-particle")
            advectType = 0;
        else if (arg == "-streamline")
        {
            advectType = 1;
        }
        else if (arg == "-streamlineS")
        {
            advectType = 1;
            stepsPerRound = static_cast<vtkm::Id>(atoi(argv[++i]));
        }
        else if (arg == "-streamlineP")
        {
            advectType = 1;
            particlesPerRound = static_cast<vtkm::Id>(atoi(argv[++i]));            
        }
        else if (arg == "-streamlineSP")
        {
            advectType = 1;
            stepsPerRound = static_cast<vtkm::Id>(atoi(argv[++i]));
            particlesPerRound = static_cast<vtkm::Id>(atoi(argv[++i]));
        }                
        else if (arg == "-file")
            dataFile = argv[++i];
        else if (arg == "-t")
            numThreads = static_cast<vtkm::Id>(atoi(argv[++i]));
        else if (arg == "-dump")
            dumpOutput = true;
        else if (arg == "-sparse")
            seeding = SPARSE;
        else if (arg == "-dense")
            seeding = DENSE;
        else if (arg == "-medium")
            seeding = MEDIUM;
        else
            std::cerr<<"Unexpected argument: "<<arg<<std::endl;
    }

    if (dataFile.size()==0)
    {
        std::cerr<<"Error: no data file specified"<<std::endl;
        return false;
    }

    //Congratulations user, we have a valid run:
    std::cerr<<pgmType<<": "<<numSeeds<<" "<<numSteps<<" "<<stepSize<<" ";
    if (advectType == 0) std::cerr<<"PP ";
    else std::cerr<<"SL ";
    std::cerr<<numThreads<<" ";
    std::cerr<<dataFile<<std::endl;
    return true;
}

int
main(int argc, char **argv)
{
    vtkm::Id numSeeds = 100, numSteps = 100, advectType = 0, numThreads=-1, stepsPerRound=-1, particlesPerRound=-1;
    vtkm::Float32 stepSize = 0.1f;
    std::string dataFile, pgmType;
    vtkm::Id seeding = SPARSE;
    bool dumpOutput = false;
    
    if (!ParseArgs(argc, argv,
                   numSeeds, numSteps, stepSize,
                   advectType, stepsPerRound, particlesPerRound,
                   numThreads, dataFile, pgmType, dumpOutput, seeding))
    {
        return -1;
    }
    
    RunTest(dataFile, numSeeds, numSteps, stepSize, numThreads, advectType, stepsPerRound, particlesPerRound, dumpOutput, seeding);
    return 0;
}
