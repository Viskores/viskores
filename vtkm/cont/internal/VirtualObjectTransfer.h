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

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename VirtualObject, typename TargetClass, typename DeviceAdapter>
struct VirtualObjectTransfer
#ifdef VTKM_DOXYGEN_ONLY
{
  /// Takes a void* to host copy of the target object, transfers it to the
  /// device, binds it to the VirtualObject, and returns a void* to an internal
  /// state structure.
  ///
  static void* Create(VirtualObject& object, const void* hostTarget);

  /// Performs cleanup of the device state used to track the VirtualObject on
  /// the device.
  ///
  static void Cleanup(void* deviceState);

  /// Update the device state with the state of target
  static void Update(void* deviceState, const void* target);
}
#endif
;
}
}
} // vtkm::cont::internal

#endif // vtkm_cont_internal_VirtualObjectTransfer_h
