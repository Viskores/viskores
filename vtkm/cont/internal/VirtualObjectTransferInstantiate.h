//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_VirtualObjectTransferInstantiate_h
#define vtk_m_cont_internal_VirtualObjectTransferInstantiate_h

#define VTKM_EXPLICITLY_INSTANTIATE_TRANSFER_FOR_DEVICE(DerivedType, DeviceDapterTagType) \
  template class vtkm::cont::internal::VirtualObjectTransfer<DerivedType, DeviceDapterTagType>

#if defined(VTKM_ENABLE_CUDA)
#include <vtkm/cont/cuda/internal/VirtualObjectTransferCuda.h>
#define VTKM_EXPLICITLY_INSTANTIATE_TRANSFER_CUDA(DerivedType) \
  VTKM_EXPLICITLY_INSTANTIATE_TRANSFER_FOR_DEVICE(DerivedType, vtkm::cont::DeviceAdapterTagCuda)
#else // defined(VTKM_ENABLE_CUDA)
#define VTKM_EXPLICITLY_INSTANTIATE_TRANSFER_CUDA(DerivedType)
#endif // defined(VTKM_ENABLE_CUDA)

#if defined(VTKM_ENABLE_KOKKOS)
#include <vtkm/cont/kokkos/internal/VirtualObjectTransferKokkos.h>
#define VTKM_EXPLICITLY_INSTANTIATE_TRANSFER_KOKKOS(DerivedType) \
  VTKM_EXPLICITLY_INSTANTIATE_TRANSFER_FOR_DEVICE(DerivedType, vtkm::cont::DeviceAdapterTagKokkos)
#else // defined(VTKM_ENABLE_KOKKOS)
#define VTKM_EXPLICITLY_INSTANTIATE_TRANSFER_KOKKOS(DerivedType)
#endif // defined(VTKM_ENABLE_KOKKOS)

#define VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(DerivedType) \
  VTKM_EXPLICITLY_INSTANTIATE_TRANSFER_CUDA(DerivedType)  \
  VTKM_EXPLICITLY_INSTANTIATE_TRANSFER_KOKKOS(DerivedType)

#endif // vtk_m_cont_internal_VirtualObjectTransferInstantiate_h
