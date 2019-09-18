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

#ifdef VTKM_ENABLE_OPENMP
VTKM_VALID_DEVICE_ADAPTER(OpenMP, VTKM_DEVICE_ADAPTER_OPENMP)
#else
VTKM_INVALID_DEVICE_ADAPTER(OpenMP, VTKM_DEVICE_ADAPTER_OPENMP)
#endif

#endif // vtk_m_cont_openmp_internal_DeviceAdapterTagOpenMP_h
