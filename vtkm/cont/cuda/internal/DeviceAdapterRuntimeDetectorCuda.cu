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
#include <vtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>

#include <vtkm/Math.h>
#include <vtkm/cont/cuda/ErrorCuda.h>

#include <cuda.h>

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

static __global__ void DetermineIfValidCudaDevice()
{
  //used only to see if we can launch kernels. It is possible to have a
  //CUDA capable device, but still fail to have CUDA support.
}
}
}
}
}

namespace vtkm
{
namespace cont
{

DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagCuda>::DeviceAdapterRuntimeDetector()
  : NumberOfDevices(0)
  , HighestArchSupported(0)
{
  static bool deviceQueryInit = false;
  static int numDevices = 0;
  static int archVersion = 0;

  if (!deviceQueryInit)
  {
    deviceQueryInit = true;

    //first query for the number of devices
    VTKM_CUDA_CALL(cudaGetDeviceCount(&numDevices));

    for (vtkm::Int32 i = 0; i < numDevices; i++)
    {
      cudaDeviceProp prop;
      VTKM_CUDA_CALL(cudaGetDeviceProperties(&prop, i));
      const vtkm::Int32 arch = (prop.major * 10) + prop.minor;
      archVersion = vtkm::Max(arch, archVersion);
    }

    //Make sure we can actually launch a kernel. This could fail for any
    //of the following reasons:
    //
    // 1. cudaErrorInsufficientDriver, caused by out of data drives
    // 2. cudaErrorDevicesUnavailable, caused by another process locking the
    //    device or somebody disabling cuda support on the device
    // 3. cudaErrorNoKernelImageForDevice we built for a compute version
    //    greater than the device we are running on
    // Most likely others that I don't even know about
    vtkm::cont::cuda::internal::DetermineIfValidCudaDevice<<<1, 1, 0, cudaStreamPerThread>>>();
    if (cudaSuccess != cudaGetLastError())
    {
      numDevices = 0;
      archVersion = 0;
    }
  }

  this->NumberOfDevices = numDevices;
  this->HighestArchSupported = archVersion;
}

bool DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagCuda>::Exists() const
{
  return this->NumberOfDevices > 0 && this->HighestArchSupported >= 20;
}
}
} // namespace vtkm::cont
