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

#include <vtkm/cont/cuda/internal/DeviceAdapterAlgorithmThrust.h>

#include <atomic>
#include <mutex>

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

VTKM_CONT_EXPORT vtkm::UInt32 getNumSMs(int dId)
{
  std::size_t index = 0;
  if (dId > 0)
  {
    index = static_cast<size_t>(dId);
  }

  //check
  static bool lookupBuilt = false;
  static std::vector<vtkm::UInt32> numSMs;

  if (!lookupBuilt)
  {
    //lock the mutex
    static std::mutex built_mutex;
    std::lock_guard<std::mutex> lock(built_mutex);

    //iterate over all devices
    int numberOfSMs = 0;
    int count = 0;
    VTKM_CUDA_CALL(cudaGetDeviceCount(&count));
    numSMs.reserve(static_cast<std::size_t>(count));
    for (int deviceId = 0; deviceId < count; ++deviceId)
    { //get the number of sm's per deviceId
      VTKM_CUDA_CALL(
        cudaDeviceGetAttribute(&numberOfSMs, cudaDevAttrMultiProcessorCount, deviceId));
      numSMs.push_back(static_cast<vtkm::UInt32>(numberOfSMs));
    }
    lookupBuilt = true;
  }
  return numSMs[index];
}

// we use cuda pinned memory to reduce the amount of synchronization
// and mem copies between the host and device.
template <>
char* DeviceAdapterAlgorithmThrust<vtkm::cont::DeviceAdapterTagCuda>::GetPinnedErrorArray(
  vtkm::Id& arraySize,
  char** hostPointer)
{
  const vtkm::Id ERROR_ARRAY_SIZE = 1024;
  static bool errorArrayInit = false;
  static char* hostPtr = nullptr;
  static char* devicePtr = nullptr;
  if (!errorArrayInit)
  {
    VTKM_CUDA_CALL(cudaMallocHost((void**)&hostPtr, ERROR_ARRAY_SIZE, cudaHostAllocMapped));
    VTKM_CUDA_CALL(cudaHostGetDevicePointer(&devicePtr, hostPtr, 0));
    errorArrayInit = true;
  }
  //set the size of the array
  arraySize = ERROR_ARRAY_SIZE;

  //specify the host pointer to the memory
  *hostPointer = hostPtr;
  (void)hostPointer;
  return devicePtr;
}

template <>
char* DeviceAdapterAlgorithmThrust<vtkm::cont::DeviceAdapterTagCuda>::SetupErrorBuffer(
  vtkm::exec::cuda::internal::TaskStrided& functor)
{
  //since the memory is pinned we can access it safely on the host
  //without a memcpy
  vtkm::Id errorArraySize = 0;
  char* hostErrorPtr = nullptr;
  char* deviceErrorPtr = GetPinnedErrorArray(errorArraySize, &hostErrorPtr);

  //clear the first character which means that we don't contain an error
  hostErrorPtr[0] = '\0';

  vtkm::exec::internal::ErrorMessageBuffer errorMessage(deviceErrorPtr, errorArraySize);
  functor.SetErrorMessageBuffer(errorMessage);

  return hostErrorPtr;
}

template <>
void DeviceAdapterAlgorithmThrust<vtkm::cont::DeviceAdapterTagCuda>::GetGridsAndBlocks(
  vtkm::UInt32& grids,
  vtkm::UInt32& blocks,
  vtkm::Id size)
{
  (void)size;
  int deviceId;
  VTKM_CUDA_CALL(cudaGetDevice(&deviceId)); //get deviceid from cuda
  grids = 32 * getNumSMs(deviceId);
  blocks = 128;
}

template <>
void DeviceAdapterAlgorithmThrust<vtkm::cont::DeviceAdapterTagCuda>::GetGridsAndBlocks(
  vtkm::UInt32& grids,
  dim3& blocks,
  const dim3& size)
{
  int deviceId;
  VTKM_CUDA_CALL(cudaGetDevice(&deviceId)); //get deviceid from cuda
  grids = 32 * getNumSMs(deviceId);

  if (size.x == 0)
  { //grids that have no x dimension
    blocks.x = 1;
    blocks.y = 16;
    blocks.z = 8;
  }
  else if (size.x > 128)
  {
    blocks.x = 64;
    blocks.y = 2;
    blocks.z = 1;
  }
  else
  { //for really small grids
    blocks.x = 8;
    blocks.y = 4;
    blocks.z = 4;
  }
}
}
}
}
}
