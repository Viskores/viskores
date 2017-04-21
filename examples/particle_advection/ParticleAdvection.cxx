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

#include <vtkm/io/reader/BOVDataSetReader.h>

#include <vector>
#include <chrono>

void RunTest(const std::string &fname,
             vtkm::Id numSeeds,
             vtkm::Id numSteps,
             vtkm::Float32 stepSize,
             vtkm::Id advectType)
{
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  typedef vtkm::Float32 FieldType;
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalConstType;
  
  vtkm::io::reader::BOVDataSetReader rdr(fname);
  vtkm::cont::DataSet ds = rdr.ReadDataSet();

  vtkm::worklet::particleadvection::RegularGridEvaluate<FieldPortalConstType, DeviceAdapter> eval(ds);

  typedef vtkm::worklet::particleadvection::RegularGridEvaluate<FieldPortalConstType, DeviceAdapter> RGEvalType;
  typedef vtkm::worklet::particleadvection::RK4Integrator<RGEvalType,FieldType,FieldPortalConstType> RK4RGType;
  
  RK4RGType rk4(eval, stepSize);

  std::vector<vtkm::Vec<FieldType,3> > seeds;
  vtkm::Bounds bounds = ds.GetCoordinateSystem().GetBounds();
  srand(314);
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

  vtkm::worklet::particleadvection::ParticleAdvectionFilter<RK4RGType,
                                                            FieldType,
                                                            DeviceAdapter> pa(rk4,seeds,ds,numSteps,(advectType==1));
  pa.run(false);
}

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        std::cerr<<"Usage "<<argv[0]<<" numSeeds numSteps stepSize type[-particle, -streamline] BOVfile"<<std::endl;
        return -1;
    }
    vtkm::Id numSeeds = atoi(argv[1]);
    vtkm::Id numSteps = atoi(argv[2]);
    vtkm::Float32 stepSize = atof(argv[3]);
    vtkm::Id advectType;
    if (std::string(argv[4]) == "-particle")
        advectType = 0;
    else if (std::string(argv[4]) == "-streamline")
        advectType = 1;
    else
    {
        std::cerr<<"Unknown particle advection type: "<<argv[4]<<std::endl;
        return -1;
    }
    std::string dataFile = argv[5];

    if (advectType == 0)
        std::cerr<<"PARTICLE   ";
    else
        std::cerr<<"STREAMLINE ";
    std::cerr<<argv[0]<<" "<<numSeeds<<" "<<numSteps<<" file= "<<dataFile<<std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    RunTest(dataFile, numSeeds, numSteps, stepSize, advectType);
    auto duration_taken = std::chrono::high_resolution_clock::now() - start;
    std::uint64_t runtime = std::chrono::duration_cast<std::chrono::milliseconds>(duration_taken).count();
    std::cerr << "Runtime = " << runtime << " ms" << std::endl;

    return 0;
}
