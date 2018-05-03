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
#ifndef vtk_m_cont_internal_VirtualObjectTransfer_h
#define vtk_m_cont_internal_VirtualObjectTransfer_h

#include <vtkm/VirtualObjectBase.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename VirtualDerivedType, typename DeviceAdapter>
struct VirtualObjectTransfer
#ifdef VTKM_DOXYGEN_ONLY
{
  /// A VirtualObjectTransfer is constructed with a pointer to the derived type that (eventually)
  /// gets transferred to the execution environment of the given DeviceAdapter.
  ///
  VTKM_CONT VirtualObjectTransfer(const VirtualDerivedType* virtualObject);

  /// \brief Transfers the virtual object to the execution environment.
  ///
  /// This method transfers the virtual object to the execution environment and returns a pointer
  /// to the object that can be used in the execution environment (but not necessarily the control
  /// environment). If the \c updateData flag is true, then the data is always copied to the
  /// execution environment (such as if the data were updated since the last call to \c
  /// PrepareForExecution). If the \c updateData flag is false and the object was already
  /// transferred previously, the previously created object is returned.
  ///
  VTKM_CONT const VirtualDerivedType* PrepareForExecution(bool updateData);

  /// \brief Frees up any resources in the execution environment.
  ///
  /// Any previously returned virtual object from \c PrepareForExecution becomes invalid.
  ///
  VTKM_CONT void ReleaseResources();
}
#endif
;
}
}
} // vtkm::cont::internal

#endif // vtkm_cont_internal_VirtualObjectTransfer_h
