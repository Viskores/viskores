//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_openmp_internal_VirtualObjectTransferOpenMP_h
#define vtk_m_cont_openmp_internal_VirtualObjectTransferOpenMP_h

#include <vtkm/cont/internal/VirtualObjectTransfer.h>
#include <vtkm/cont/internal/VirtualObjectTransferShareWithControl.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#ifdef VTKM_NO_DEPRECATED_VIRTUAL
#error "This header should not be included when VTKM_NO_DEPRECATED_VIRTUAL is set."
#endif //VTKM_NO_DEPRECATED_VIRTUAL

namespace vtkm
{
namespace cont
{
namespace internal
{

VTKM_DEPRECATED_SUPPRESS_BEGIN

template <typename VirtualDerivedType>
struct VirtualObjectTransfer<VirtualDerivedType, vtkm::cont::DeviceAdapterTagOpenMP> final
  : VirtualObjectTransferShareWithControl<VirtualDerivedType>
{
  using VirtualObjectTransferShareWithControl<
    VirtualDerivedType>::VirtualObjectTransferShareWithControl;
};

VTKM_DEPRECATED_SUPPRESS_END

}
}
} // vtkm::cont::internal



#endif // vtk_m_cont_openmp_internal_VirtualObjectTransferOpenMP_h
