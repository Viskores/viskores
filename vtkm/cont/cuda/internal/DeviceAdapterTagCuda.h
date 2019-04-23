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

//We always create the cuda tag when included, but we only mark it as
//a valid tag when VTKM_CUDA is true. This is for easier development
//of multi-backend systems
#if defined(VTKM_CUDA) && defined(VTKM_ENABLE_CUDA)
VTKM_VALID_DEVICE_ADAPTER(Cuda, VTKM_DEVICE_ADAPTER_CUDA);
#elif defined(VTKM_ENABLE_CUDA) && !defined(VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)
#error When VTK-m is build with CUDA enabled all compilation units that include DeviceAdapterTagCuda must use the cuda compiler
#else
VTKM_INVALID_DEVICE_ADAPTER(Cuda, VTKM_DEVICE_ADAPTER_CUDA);
#endif

#endif //vtk_m_cont_cuda_internal_DeviceAdapterTagCuda_h
