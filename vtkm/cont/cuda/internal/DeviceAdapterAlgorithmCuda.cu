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

#include <vtkm/cont/cuda/internal/DeviceAdapterAlgorithmCuda.h>

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
  static std::once_flag lookupBuiltFlag;
  static std::vector<vtkm::UInt32> numSMs;

  std::call_once(lookupBuiltFlag, []() {
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
  });
  return numSMs[index];
}
}
} // end namespace cuda::internal

// we use cuda pinned memory to reduce the amount of synchronization
// and mem copies between the host and device.
auto DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>::GetPinnedErrorArray()
  -> const PinnedErrorArray&
{
  constexpr vtkm::Id ERROR_ARRAY_SIZE = 1024;
  static thread_local PinnedErrorArray local;

  if (!local.HostPtr)
  {
    VTKM_CUDA_CALL(cudaMallocHost((void**)&local.HostPtr, ERROR_ARRAY_SIZE, cudaHostAllocMapped));
    VTKM_CUDA_CALL(cudaHostGetDevicePointer(&local.DevicePtr, local.HostPtr, 0));
    local.HostPtr[0] = '\0'; // clear
    local.Size = ERROR_ARRAY_SIZE;
  }

  return local;
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>::SetupErrorBuffer(
  vtkm::exec::cuda::internal::TaskStrided& functor)
{
  auto pinnedArray = GetPinnedErrorArray();
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(pinnedArray.DevicePtr, pinnedArray.Size);
  functor.SetErrorMessageBuffer(errorMessage);
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>::CheckForErrors()
{
  auto pinnedArray = GetPinnedErrorArray();
  if (pinnedArray.HostPtr[0] != '\0')
  {
    VTKM_CUDA_CALL(cudaStreamSynchronize(cudaStreamPerThread));
    auto excep = vtkm::cont::ErrorExecution(pinnedArray.HostPtr);
    pinnedArray.HostPtr[0] = '\0'; // clear
    throw excep;
  }
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>::GetGridsAndBlocks(
  vtkm::UInt32& grids,
  vtkm::UInt32& blocks,
  vtkm::Id size)
{
  (void)size;
  int deviceId;
  VTKM_CUDA_CALL(cudaGetDevice(&deviceId)); //get deviceid from cuda
  grids = 32 * cuda::internal::getNumSMs(deviceId);
  blocks = 128;
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>::GetGridsAndBlocks(
  vtkm::UInt32& grids,
  dim3& blocks,
  const dim3& size)
{
  int deviceId;
  VTKM_CUDA_CALL(cudaGetDevice(&deviceId)); //get deviceid from cuda
  grids = 32 * cuda::internal::getNumSMs(deviceId);

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
} // end namespace vtkm::cont
