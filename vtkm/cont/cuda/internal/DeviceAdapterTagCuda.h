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
#ifdef VTKM_CUDA
VTKM_VALID_DEVICE_ADAPTER(Cuda, VTKM_DEVICE_ADAPTER_CUDA);
#else
VTKM_INVALID_DEVICE_ADAPTER(Cuda, VTKM_DEVICE_ADAPTER_CUDA);
#endif

#endif //vtk_m_cont_cuda_internal_DeviceAdapterTagCuda_h
