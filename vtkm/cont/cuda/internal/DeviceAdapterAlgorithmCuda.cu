//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/cuda/internal/DeviceAdapterAlgorithmCuda.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <mutex>

#include <cuda.h>

// minwindef.h on Windows creates a preprocessor macro named PASCAL, which messes this up.
#ifdef PASCAL
#undef PASCAL
#endif

namespace vtkm
{
namespace cont
{
namespace cuda
{

static vtkm::cont::cuda::ScheduleParameters (
  *ComputeFromEnv)(const char*, int, int, int, int, int) = nullptr;

//Use the provided function as the the compute function for ScheduleParameterBuilder
VTKM_CONT_EXPORT void InitScheduleParameters(
  vtkm::cont::cuda::ScheduleParameters (*function)(const char*, int, int, int, int, int))
{
  ComputeFromEnv = function;
}

namespace internal
{

//These represent the best block/threads-per for scheduling on each GPU
static std::vector<std::pair<int, int>> scheduling_1d_parameters;
static std::vector<std::pair<int, dim3>> scheduling_2d_parameters;
static std::vector<std::pair<int, dim3>> scheduling_3d_parameters;

struct VTKM_CONT_EXPORT ScheduleParameterBuilder
{
  //This represents information that is used to compute the best
  //ScheduleParameters for a given GPU
  enum struct GPU_STRATA
  {
    ENV = 0,
    OLDER = 5,
    PASCAL = 6,
    VOLTA = 7,
    PASCAL_HPC = 6000,
    VOLTA_HPC = 7000
  };

  std::map<GPU_STRATA, vtkm::cont::cuda::ScheduleParameters> Presets;
  std::function<vtkm::cont::cuda::ScheduleParameters(const char*, int, int, int, int, int)> Compute;

  // clang-format off
  // The presets for [one,two,three]_d_blocks are before we multiply by the number of SMs on the hardware
  ScheduleParameterBuilder()
    : Presets{
      { GPU_STRATA::ENV,        {  0,   0,  0, {  0,  0, 0 },  0, { 0, 0, 0 } } }, //use env settings
      { GPU_STRATA::OLDER,
                                { 32, 128,  8, { 16, 16, 1 }, 32, { 8, 8, 4 } } }, //VTK-m default for less than pascal
      { GPU_STRATA::PASCAL,     { 32, 128,  8, { 16, 16, 1 }, 32, { 8, 8, 4 } } }, //VTK-m default for pascal
      { GPU_STRATA::VOLTA,      { 32, 128,  8, { 16, 16, 1 }, 32, { 8, 8, 4 } } }, //VTK-m default for volta
      { GPU_STRATA::PASCAL_HPC, { 32, 256, 16, { 16, 16, 1 }, 64, { 8, 8, 4 } } }, //P100
      { GPU_STRATA::VOLTA_HPC,  { 32, 256, 16, { 16, 16, 1 }, 64, { 8, 8, 4 } } }, //V100
    }
    , Compute(nullptr)
  {
    if (vtkm::cont::cuda::ComputeFromEnv != nullptr)
    {
      this->Compute = vtkm::cont::cuda::ComputeFromEnv;
    }
    else
    {
      this->Compute = [=] (const char* name, int major, int minor,
                          int numSMs, int maxThreadsPerSM, int maxThreadsPerBlock) -> ScheduleParameters  {
        return this->ComputeFromPreset(name, major, minor, numSMs, maxThreadsPerSM, maxThreadsPerBlock); };
    }
  }
  // clang-format on

  vtkm::cont::cuda::ScheduleParameters ComputeFromPreset(const char* name,
                                                         int major,
                                                         int minor,
                                                         int numSMs,
                                                         int maxThreadsPerSM,
                                                         int maxThreadsPerBlock)
  {
    (void)minor;
    (void)maxThreadsPerSM;
    (void)maxThreadsPerBlock;

    const constexpr int GPU_STRATA_MAX_GEN = 7;
    const constexpr int GPU_STRATA_MIN_GEN = 5;
    int strataAsInt = std::min(major, GPU_STRATA_MAX_GEN);
    strataAsInt = std::max(strataAsInt, GPU_STRATA_MIN_GEN);
    if (strataAsInt > GPU_STRATA_MIN_GEN)
    { //only pascal and above have fancy

      //Currently the only
      bool is_tesla = (0 == std::strncmp("Tesla", name, 4)); //see if the name starts with Tesla
      if (is_tesla)
      {
        strataAsInt *= 1000; //tesla modifier
      }
    }

    auto preset = this->Presets.find(static_cast<GPU_STRATA>(strataAsInt));
    ScheduleParameters params = preset->second;
    params.one_d_blocks = params.one_d_blocks * numSMs;
    params.two_d_blocks = params.two_d_blocks * numSMs;
    params.three_d_blocks = params.three_d_blocks * numSMs;
    return params;
  }
};

VTKM_CONT_EXPORT void SetupKernelSchedulingParameters()
{
  //check flag
  static std::once_flag lookupBuiltFlag;

  std::call_once(lookupBuiltFlag, []() {
    ScheduleParameterBuilder builder;
    //iterate over all devices
    int count = 0;
    VTKM_CUDA_CALL(cudaGetDeviceCount(&count));
    for (int deviceId = 0; deviceId < count; ++deviceId)
    {
      cudaDeviceProp deviceProp;
      cudaGetDeviceProperties(&deviceProp, deviceId);

      ScheduleParameters params = builder.Compute(deviceProp.name,
                                                  deviceProp.major,
                                                  deviceProp.minor,
                                                  deviceProp.multiProcessorCount,
                                                  deviceProp.maxThreadsPerMultiProcessor,
                                                  deviceProp.maxThreadsPerBlock);
      scheduling_1d_parameters.emplace_back(params.one_d_blocks, params.one_d_threads_per_block);
      scheduling_2d_parameters.emplace_back(params.two_d_blocks, params.two_d_threads_per_block);
      scheduling_3d_parameters.emplace_back(params.three_d_blocks,
                                            params.three_d_threads_per_block);
    }
  });
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

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>::GetBlocksAndThreads(
  vtkm::UInt32& blocks,
  vtkm::UInt32& threadsPerBlock,
  vtkm::Id size)
{
  (void)size;
  vtkm::cont::cuda::internal::SetupKernelSchedulingParameters();

  int deviceId;
  VTKM_CUDA_CALL(cudaGetDevice(&deviceId)); //get deviceid from cuda
  const auto& params = cuda::internal::scheduling_1d_parameters[static_cast<size_t>(deviceId)];
  blocks = params.first;
  threadsPerBlock = params.second;
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>::GetBlocksAndThreads(
  vtkm::UInt32& blocks,
  dim3& threadsPerBlock,
  const dim3& size)
{
  vtkm::cont::cuda::internal::SetupKernelSchedulingParameters();

  int deviceId;
  VTKM_CUDA_CALL(cudaGetDevice(&deviceId)); //get deviceid from cuda
  if (size.z <= 1)
  { //2d images
    const auto& params = cuda::internal::scheduling_2d_parameters[static_cast<size_t>(deviceId)];
    blocks = params.first;
    threadsPerBlock = params.second;
  }
  else
  { //3d images
    const auto& params = cuda::internal::scheduling_3d_parameters[static_cast<size_t>(deviceId)];
    blocks = params.first;
    threadsPerBlock = params.second;
  }
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>::LogKernelLaunch(
  const cudaFuncAttributes& func_attrs,
  const std::type_info& worklet_info,
  vtkm::UInt32 blocks,
  vtkm::UInt32 threadsPerBlock,
  vtkm::Id)
{
  (void)func_attrs;
  (void)blocks;
  (void)threadsPerBlock;
  std::string name = vtkm::cont::TypeToString(worklet_info);
  VTKM_LOG_F(vtkm::cont::LogLevel::KernelLaunches,
             "Launching 1D kernel %s on CUDA [ptx=%i, blocks=%i, threadsPerBlock=%i]",
             name.c_str(),
             (func_attrs.ptxVersion * 10),
             blocks,
             threadsPerBlock);
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>::LogKernelLaunch(
  const cudaFuncAttributes& func_attrs,
  const std::type_info& worklet_info,
  vtkm::UInt32 blocks,
  dim3 threadsPerBlock,
  const dim3&)
{
  (void)func_attrs;
  (void)blocks;
  (void)threadsPerBlock;
  std::string name = vtkm::cont::TypeToString(worklet_info);
  VTKM_LOG_F(vtkm::cont::LogLevel::KernelLaunches,
             "Launching 3D kernel %s on CUDA [ptx=%i, blocks=%i, threadsPerBlock=%i, %i, %i]",
             name.c_str(),
             (func_attrs.ptxVersion * 10),
             blocks,
             threadsPerBlock.x,
             threadsPerBlock.y,
             threadsPerBlock.z);
}
}
} // end namespace vtkm::cont
