//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_openmp_DeviceAdapterOpenMP_h
#define vtk_m_cont_openmp_DeviceAdapterOpenMP_h

#include <vtkm/cont/openmp/internal/DeviceAdapterRuntimeDetectorOpenMP.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#ifdef VTKM_ENABLE_OPENMP
#include <vtkm/cont/openmp/internal/ArrayManagerExecutionOpenMP.h>
#include <vtkm/cont/openmp/internal/AtomicInterfaceExecutionOpenMP.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterAlgorithmOpenMP.h>
#include <vtkm/cont/openmp/internal/VirtualObjectTransferOpenMP.h>
#endif

#endif //vtk_m_cont_openmp_DeviceAdapterOpenMP_h
