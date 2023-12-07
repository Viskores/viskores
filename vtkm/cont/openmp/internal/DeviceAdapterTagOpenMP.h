//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_openmp_internal_DeviceAdapterTagOpenMP_h
#define vtk_m_cont_openmp_internal_DeviceAdapterTagOpenMP_h

#include <vtkm/cont/DeviceAdapterTag.h>

/// @struct vtkm::cont::DeviceAdapterTagOpenMP
/// @brief Tag for a device adapter that uses OpenMP compiler extensions to
/// run algorithms on multiple threads.
///
/// For this device to work, VTK-m must be configured to use OpenMP and the code
/// must be compiled with a compiler that supports OpenMP pragmas. This tag is
/// defined in `vtkm/cont/openmp/DeviceAdapterOpenMP.h`.

#ifdef VTKM_ENABLE_OPENMP
VTKM_VALID_DEVICE_ADAPTER(OpenMP, VTKM_DEVICE_ADAPTER_OPENMP)
#else
VTKM_INVALID_DEVICE_ADAPTER(OpenMP, VTKM_DEVICE_ADAPTER_OPENMP)
#endif

#endif // vtk_m_cont_openmp_internal_DeviceAdapterTagOpenMP_h
