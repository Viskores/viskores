//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cuda_interop_SetOpenGLDevice_h
#define vtk_m_cuda_interop_SetOpenGLDevice_h

#include <cuda.h>
#include <cuda_gl_interop.h>

#include <vtkm/cont/ErrorExecution.h>

namespace vtkm
{
namespace interop
{
namespace cuda
{

static void SetCudaGLDevice(int id)
{
//With Cuda 5.0 cudaGLSetGLDevice is deprecated and shouldn't be needed
//anymore. But it seems that macs still require you to call it or we
//segfault
#ifdef __APPLE__
  cudaError_t cError = cudaGLSetGLDevice(id);
#else
  cudaError_t cError = cudaSetDevice(id);
#endif
  if (cError != cudaSuccess)
  {
    std::string cuda_error_msg("Unable to setup cuda/opengl interop. Error: ");
    cuda_error_msg.append(cudaGetErrorString(cError));
    throw vtkm::cont::ErrorExecution(cuda_error_msg);
  }
}
}
}
} //namespace

#endif //vtk_m_cuda_interop_SetOpenGLDevice_h
