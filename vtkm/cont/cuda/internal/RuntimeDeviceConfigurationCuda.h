//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_RuntimeDeviceConfigurationCuda_h
#define vtk_m_cont_cuda_internal_RuntimeDeviceConfigurationCuda_h

#include <vtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>

#include <vtkm/cont/Logging.h>
#include <vtkm/cont/cuda/ErrorCuda.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <cuda.h>
VTKM_THIRDPARTY_POST_INCLUDE

#include <vector>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagCuda>
  : public vtkm::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagCuda>()
  {
    this->CudaDeviceCount = 0;
    this->CudaProp.clear();
    vtkm::cont::DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagCuda> detector;
    if (detector.Exists())
    {
      try
      {
        int tmp;
        VTKM_CUDA_CALL(cudaGetDeviceCount(&tmp));
        this->CudaDeviceCount = tmp;
        this->CudaProp.resize(this->CudaDeviceCount);
        for (int i = 0; i < this->CudaDeviceCount; ++i)
        {
          VTKM_CUDA_CALL(cudaGetDeviceProperties(&this->CudaProp[i], i));
        }
      }
      catch (...)
      {
        VTKM_LOG_F(vtkm::cont::LogLevel::Error,
                   "Error retrieving CUDA device information. Disabling.");
        this->CudaDeviceCount = 0;
      }
    }
  }

  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override final
  {
    return vtkm::cont::DeviceAdapterTagCuda{};
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(
    const vtkm::Id& value) override final
  {
    if (value >= this->CudaDeviceCount)
    {
      VTKM_LOG_S(
        vtkm::cont::LogLevel::Error,
        "Failed to set CudaDeviceInstance, supplied id exceeds the number of available devices: "
          << value << " >= " << this->CudaDeviceCount);
      return RuntimeDeviceConfigReturnCode::INVALID_VALUE;
    }
    VTKM_CUDA_CALL(cudaSetDevice(value));
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(
    vtkm::Id& value) const override final
  {
    int tmp;
    VTKM_CUDA_CALL(cudaGetDevice(&tmp));
    value = tmp;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetMaxDevices(
    vtkm::Id& value) const override final
  {
    value = this->CudaDeviceCount;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  /// A function only available for use by the Cuda instance of this class
  /// Used to grab the CudaDeviceProp structs for all available devices
  VTKM_CONT RuntimeDeviceConfigReturnCode
  GetCudaDeviceProp(std::vector<cudaDeviceProp>& value) const
  {
    value = CudaProp;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

private:
  std::vector<cudaDeviceProp> CudaProp;
  vtkm::Id CudaDeviceCount;
};
} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_cuda_internal_RuntimeDeviceConfigurationCuda_h
