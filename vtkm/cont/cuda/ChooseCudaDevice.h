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
#ifndef vtk_m_cont_cuda_ChooseCudaDevice_h
#define vtk_m_cont_cuda_ChooseCudaDevice_h

#include <vtkm/cont/ErrorExecution.h>

#include <cuda.h>
#include <algorithm>
#include <vector>

namespace vtkm{
namespace cuda{
namespace cont {

namespace {
struct compute_info
{
  compute_info(cudaDeviceProp prop, int index)
  {
    this->Index = index;
    this->Major = prop.major;

    this->MemorySize = prop.totalGlobalMem;
    this->Performance = prop.multiProcessorCount *
        prop.maxThreadsPerMultiProcessor *
        (prop.clockRate / 100000.0);

    //9999 is equal to emulation make sure it is a super bad device
    if(this->Major >= 9999)
    {
      this->Major = -1;
      this->Performance = -1;
    }
  }

  //sort from fastest to slowest
  bool operator<(const compute_info other) const
  {
    //if we are both SM3 or greater check performance
    //if we both the same SM level check performance
    if( (this->Major >= 3 && other.Major >= 3) ||
        (this->Major == other.Major) )
    {
      return betterPerfomance(other);
    }
    //prefer the greater SM otherwise
    return this->Major > other.Major;
  }

  bool betterPerfomance(const compute_info other) const
  {
    if ( this->Performance == other.Performance)
    {
      if( this->MemorySize == other.MemorySize )
      {
        //prefer first device over second device
        //this will be subjective I bet
        return this->Index < other.Index;
      }
      return this->MemorySize > other.MemorySize;
    }
    return this->Performance > other.Performance;
  }

  int GetIndex() const { return Index; }

private:
  int Index;
  int Major;
  size_t MemorySize;
  double Performance;
};

}

///Returns the fastest cuda device id that the current system has
///A result of zero means no cuda device has been found
static int FindFastestDeviceId()
{
  //get the number of devices and store information
  int numberOfDevices=0;
  cudaGetDeviceCount(&numberOfDevices);

  std::vector<compute_info> devices;
  for(int i=0; i < numberOfDevices; ++i)
    {
    cudaDeviceProp properties;
    cudaGetDeviceProperties(&properties, i);
    if(properties.computeMode != cudaComputeModeProhibited)
      {
      //only add devices that have compute mode allowed
      devices.push_back( compute_info(properties,i) );
      }
    }

  //sort from fastest to slowest
  std::sort(devices.begin(),devices.end());

  int device=0;
  if(devices.size()> 0)
    {
    device = devices.front().GetIndex();
    }
  return device;
}


//choose a cuda compute device. This can't be used if you are setting
//up open gl interop
static void SetCudaDevice(int id)
{
  cudaError_t cError = cudaSetDevice(id);
  if(cError != cudaSuccess)
    {
    std::string cuda_error_msg(
                 "Unable to bind to the given cuda device. Error: ");
    cuda_error_msg.append(cudaGetErrorString(cError));
    throw vtkm::cont::ErrorExecution(cuda_error_msg);
    }
}


}
}
} //namespace

#endif
