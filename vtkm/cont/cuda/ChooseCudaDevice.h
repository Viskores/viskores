//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_ChooseCudaDevice_h
#define vtk_m_cont_cuda_ChooseCudaDevice_h

#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/cont/RuntimeDeviceInformation.h>
#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/cuda/internal/RuntimeDeviceConfigurationCuda.h>

#include <algorithm>
#include <set>
#include <vector>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <cuda.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace cont
{
namespace cuda
{

namespace
{
struct compute_info
{
  compute_info(cudaDeviceProp prop, int index)
  {
    this->Index = index;
    this->Major = prop.major;

    this->MemorySize = prop.totalGlobalMem;
    this->Performance =
      prop.multiProcessorCount * prop.maxThreadsPerMultiProcessor * (prop.clockRate / 100000.0);

    //9999 is equal to emulation make sure it is a super bad device
    if (this->Major >= 9999)
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
    if ((this->Major >= 3 && other.Major >= 3) || (this->Major == other.Major))
    {
      return betterPerformance(other);
    }
    //prefer the greater SM otherwise
    return this->Major > other.Major;
  }

  bool betterPerformance(const compute_info other) const
  {
    if (this->Performance == other.Performance)
    {
      if (this->MemorySize == other.MemorySize)
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
  auto cudaDeviceConfig = dynamic_cast<
    vtkm::cont::internal::RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagCuda>&>(
    vtkm::cont::RuntimeDeviceInformation{}.GetRuntimeConfiguration(
      vtkm::cont::DeviceAdapterTagCuda()));
  vtkm::Id numDevices;
  cudaDeviceConfig.GetMaxDevices(numDevices);

  // multiset stores elements in sorted order (allows duplicate values)
  std::multiset<compute_info> devices;
  std::vector<cudaDeviceProp> cudaProp;
  cudaDeviceConfig.GetCudaDeviceProp(cudaProp);
  for (int i = 0; i < numDevices; ++i)
  {
    if (cudaProp[i].computeMode != cudaComputeModeProhibited)
    {
      devices.emplace(cudaProp[i], i);
    }
  }

  return devices.size() > 0 ? devices.begin()->GetIndex() : 0;
}

/// Sets the current cuda device to the value returned by FindFastestDeviceId
static void SetFastestDeviceId()
{
  auto deviceId = FindFastestDeviceId();
  vtkm::cont::RuntimeDeviceInformation{}
    .GetRuntimeConfiguration(vtkm::cont::DeviceAdapterTagCuda())
    .SetDeviceInstance(deviceId);
}

}
}
} //namespace

#endif
