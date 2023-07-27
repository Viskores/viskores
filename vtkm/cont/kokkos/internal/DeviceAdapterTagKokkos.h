//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_internal_DeviceAdapterTagKokkos_h
#define vtk_m_cont_kokkos_internal_DeviceAdapterTagKokkos_h

#include <vtkm/cont/DeviceAdapterTag.h>

/// @struct vtkm::cont::DeviceAdapterTagKokkos
/// @brief Tag for a device adapter that uses the Kokkos library to run algorithms in parallel.
///
/// For this device to work, VTK-m must be configured to use Kokkos and the executable
/// must be linked to the Kokkos libraries. VTK-m will use the default execution space
/// of the provided kokkos library build. This tag is defined in
/// `vtkm/cont/kokkos/DeviceAdapterKokkos.h`.

//We always create the kokkos tag when included, but we only mark it as
//a valid tag when VTKM_ENABLE_KOKKOS is true. This is for easier development
//of multi-backend systems
#if defined(VTKM_ENABLE_KOKKOS) &&                       \
  ((!defined(VTKM_KOKKOS_CUDA) || defined(VTKM_CUDA)) || \
   !defined(VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG))
VTKM_VALID_DEVICE_ADAPTER(Kokkos, VTKM_DEVICE_ADAPTER_KOKKOS);
#else
VTKM_INVALID_DEVICE_ADAPTER(Kokkos, VTKM_DEVICE_ADAPTER_KOKKOS);
#endif

#endif // vtk_m_cont_kokkos_internal_DeviceAdapterTagKokkos_h
