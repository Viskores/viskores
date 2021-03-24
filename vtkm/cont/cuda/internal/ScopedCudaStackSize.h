//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_ScopedCudaStackSize_h
#define vtk_m_cont_cuda_internal_ScopedCudaStackSize_h

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

/// \brief RAII helper for temporarily changing CUDA stack size in an
/// exception-safe way.
struct ScopedCudaStackSize
{
  ScopedCudaStackSize(std::size_t newStackSize)
  {
    cudaDeviceGetLimit(&this->OldStackSize, cudaLimitStackSize);
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Temporarily changing Cuda stack size from "
                 << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(this->OldStackSize))
                 << " to "
                 << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(newStackSize)));
    cudaDeviceSetLimit(cudaLimitStackSize, newStackSize);
  }

  ~ScopedCudaStackSize()
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Restoring Cuda stack size to " << vtkm::cont::GetHumanReadableSize(
                 static_cast<vtkm::UInt64>(this->OldStackSize)));
    cudaDeviceSetLimit(cudaLimitStackSize, this->OldStackSize);
  }

  // Disable copy
  ScopedCudaStackSize(const ScopedCudaStackSize&) = delete;
  ScopedCudaStackSize& operator=(const ScopedCudaStackSize&) = delete;

private:
  std::size_t OldStackSize;
};
}
}
}
} // vtkm::cont::cuda::internal

#endif // vtk_m_cont_cuda_internal_ScopedCudaStackSize_h
