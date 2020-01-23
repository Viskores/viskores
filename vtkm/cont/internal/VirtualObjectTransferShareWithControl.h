//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_VirtualObjectTransferShareWithControl_h
#define vtk_m_cont_internal_VirtualObjectTransferShareWithControl_h

#include <vtkm/StaticAssert.h>
#include <vtkm/VirtualObjectBase.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename VirtualDerivedType>
struct VirtualObjectTransferShareWithControl
{
  VTKM_CONT VirtualObjectTransferShareWithControl(const VirtualDerivedType* virtualObject)
    : VirtualObject(virtualObject)
  {
  }

  VTKM_CONT const VirtualDerivedType* PrepareForExecution(bool vtkmNotUsed(updateData))
  {
    return this->VirtualObject;
  }

  VTKM_CONT void ReleaseResources() {}

private:
  const VirtualDerivedType* VirtualObject;
};
}
}
} // vtkm::cont::internal

#endif // vtk_m_cont_internal_VirtualObjectTransferShareWithControl_h
