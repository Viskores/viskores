//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_ErrorCuda_h
#define vtk_m_cont_cuda_ErrorCuda_h

#include <vtkm/Types.h>
#include <vtkm/cont/Error.h>

#include <cuda.h>

#include <sstream>

/// A macro that can be used to check to see if there are any unchecked
/// CUDA errors. Will throw an ErrorCuda if there are.
///
#define VTKM_CUDA_CHECK_ASYNCHRONOUS_ERROR()                                                       \
  VTKM_SWALLOW_SEMICOLON_PRE_BLOCK                                                                 \
  {                                                                                                \
    const cudaError_t vtkm_cuda_check_async_error = cudaGetLastError();                            \
    if (vtkm_cuda_check_async_error != cudaSuccess)                                                \
    {                                                                                              \
      throw ::vtkm::cont::cuda::ErrorCuda(                                                         \
        vtkm_cuda_check_async_error, __FILE__, __LINE__, "Unchecked asynchronous error");          \
    }                                                                                              \
  }                                                                                                \
  VTKM_SWALLOW_SEMICOLON_POST_BLOCK

/// A macro that can be wrapped around a CUDA command and will throw an
/// ErrorCuda exception if the CUDA command fails.
///
#define VTKM_CUDA_CALL(command)                                                                    \
  VTKM_CUDA_CHECK_ASYNCHRONOUS_ERROR();                                                            \
  VTKM_SWALLOW_SEMICOLON_PRE_BLOCK                                                                 \
  {                                                                                                \
    const cudaError_t vtkm_cuda_call_error = command;                                              \
    if (vtkm_cuda_call_error != cudaSuccess)                                                       \
    {                                                                                              \
      throw ::vtkm::cont::cuda::ErrorCuda(vtkm_cuda_call_error, __FILE__, __LINE__, #command);     \
    }                                                                                              \
  }                                                                                                \
  VTKM_SWALLOW_SEMICOLON_POST_BLOCK

namespace vtkm
{
namespace cont
{
namespace cuda
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This error is thrown whenever an unidentified CUDA runtime error is
/// encountered.
///
class VTKM_ALWAYS_EXPORT ErrorCuda : public vtkm::cont::Error
{
public:
  ErrorCuda(cudaError_t error)
  {
    std::stringstream message;
    message << "CUDA Error: " << cudaGetErrorString(error);
    this->SetMessage(message.str());
  }

  ErrorCuda(cudaError_t error,
            const std::string& file,
            vtkm::Id line,
            const std::string& description)
  {
    std::stringstream message;
    message << "CUDA Error: " << cudaGetErrorString(error) << std::endl
            << description << " @ " << file << ":" << line;
    this->SetMessage(message.str());
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace vtkm::cont:cuda

#endif //vtk_m_cont_cuda_ErrorCuda_h
