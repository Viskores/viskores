//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_DeviceAdapterKokkos_h
#define vtk_m_cont_kokkos_DeviceAdapterKokkos_h

#include <vtkm/cont/kokkos/internal/DeviceAdapterTagKokkos.h>

#if defined(VTKM_ENABLE_KOKKOS)

#if !defined(VTKM_KOKKOS_CUDA) || defined(VTKM_CUDA)

#include <vtkm/cont/kokkos/internal/DeviceAdapterAlgorithmKokkos.h>
#include <vtkm/cont/kokkos/internal/DeviceAdapterMemoryManagerKokkos.h>
#include <vtkm/cont/kokkos/internal/DeviceAdapterRuntimeDetectorKokkos.h>
#include <vtkm/cont/kokkos/internal/RuntimeDeviceConfigurationKokkos.h>
#ifndef VTKM_NO_DEPRECATED_VIRTUAL
#include <vtkm/cont/kokkos/internal/VirtualObjectTransferKokkos.h>
#endif //VTKM_NO_DEPRECATED_VIRTUAL

#else // !defined(VTKM_KOKKOS_CUDA) || defined(VTKM_CUDA)

#if !defined(VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)
#error When VTK-m is built with Kokkoas with CUDA enabled, all compilation units that include DeviceAdapterTagKokkos must use the cuda compiler
#endif // !defined(VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)

#endif // !defined(VTKM_KOKKOS_CUDA) || defined(VTKM_CUDA)

#endif // defined(VTKM_ENABLE_KOKKOS)

#endif //vtk_m_cont_kokkos_DeviceAdapterKokkos_h
