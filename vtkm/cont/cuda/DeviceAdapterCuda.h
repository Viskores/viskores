//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_DeviceAdapterCuda_h
#define vtk_m_cont_cuda_DeviceAdapterCuda_h

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

#ifdef VTKM_ENABLE_CUDA

#ifdef VTKM_CUDA

//This is required to be first so that we get patches for thrust included
//in the correct order
#include <vtkm/exec/cuda/internal/ThrustPatches.h>

#include <vtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterAlgorithmCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterMemoryManagerCuda.h>
#include <vtkm/cont/cuda/internal/VirtualObjectTransferCuda.h>

#else // !VTKM_CUDA

#ifndef VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG
#error When VTK-m is built with CUDA enabled all compilation units that include DeviceAdapterTagCuda must use the cuda compiler
#endif //!VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG

#endif // !VTKM_CUDA

#endif // VTKM_ENABLE_CUDA

#endif //vtk_m_cont_cuda_DeviceAdapterCuda_h
