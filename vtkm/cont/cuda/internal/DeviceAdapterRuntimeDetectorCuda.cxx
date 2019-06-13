//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>

#ifdef VTKM_CUDA

#include <mutex>

#include <cuda.h>
#include <vtkm/Math.h>
#include <vtkm/cont/cuda/ErrorCuda.h>

namespace
{
static std::once_flag deviceQueryFlag;
static int numDevices = 0;
static int archVersion = 0;

void queryNumberOfDevicesandHighestArchSupported(vtkm::Int32& nod, vtkm::Int32& has)
{
  std::call_once(deviceQueryFlag, []() {
    //first query for the number of devices
    auto res = cudaGetDeviceCount(&numDevices);
    if (res != cudaSuccess)
    {
      numDevices = 0;
    }

    for (vtkm::Int32 i = 0; i < numDevices; i++)
    {
      cudaDeviceProp prop;
      res = cudaGetDeviceProperties(&prop, i);
      if (res == cudaSuccess)
      {
        const vtkm::Int32 arch = (prop.major * 10) + prop.minor;
        archVersion = vtkm::Max(arch, archVersion);
      }
    }
  });
  nod = numDevices;
  has = archVersion;
}
} // anonymous namspace

#endif

namespace vtkm
{
namespace cont
{

DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagCuda>::DeviceAdapterRuntimeDetector()
  : NumberOfDevices(0)
  , HighestArchSupported(0)
{
#ifdef VTKM_CUDA
  queryNumberOfDevicesandHighestArchSupported(this->NumberOfDevices, this->HighestArchSupported);
#endif
}

bool DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagCuda>::Exists() const
{
  return this->NumberOfDevices > 0 && this->HighestArchSupported >= 30;
}
}
} // namespace vtkm::cont
