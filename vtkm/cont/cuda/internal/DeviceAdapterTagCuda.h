//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterTagCuda_h
#define vtk_m_cont_cuda_internal_DeviceAdapterTagCuda_h

#include <vtkm/cont/DeviceAdapterTag.h>

/// @struct vtkm::cont::DeviceAdapterTagCuda
/// @brief Tag for a device adapter that uses a CUDA capable GPU device.
///
/// For this device to work, VTK-m must be configured to use CUDA and the code must
/// be compiled by the CUDA `nvcc` compiler. This tag is defined in
/// `vtkm/cont/cuda/DeviceAdapterCuda.h`.

// We always create the cuda tag when included, but we only mark it as a valid tag when
// VTKM_ENABLE_CUDA is true. This is for easier development of multi-backend systems.
//
// We usually mark the Cuda device as valid if VTKM_ENABLE_CUDA even if not compiling with Cuda.
// This is because you can still call a method in a different translation unit that is compiled
// with Cuda. However, if VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG is set and we are not compiling with
// Cuda, then the device is marked invalid. This is so you can specifically compile CPU stuff even
// if other units are using Cuda.
#if defined(VTKM_ENABLE_CUDA) && !defined(VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)
VTKM_VALID_DEVICE_ADAPTER(Cuda, VTKM_DEVICE_ADAPTER_CUDA);
#else
VTKM_INVALID_DEVICE_ADAPTER(Cuda, VTKM_DEVICE_ADAPTER_CUDA);
#endif

#endif //vtk_m_cont_cuda_internal_DeviceAdapterTagCuda_h
