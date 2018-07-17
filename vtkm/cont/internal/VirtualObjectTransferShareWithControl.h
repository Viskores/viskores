//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
