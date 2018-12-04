//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_tbb_internal_ExecutionArrayInterfaceBasicTBB_h
#define vtk_m_cont_tbb_internal_ExecutionArrayInterfaceBasicTBB_h

#include <vtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>

// These must be placed in the vtkm::cont::internal namespace so that
// the template can be found.

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
struct VTKM_CONT_EXPORT ExecutionArrayInterfaceBasic<DeviceAdapterTagTBB> final
  : public ExecutionArrayInterfaceBasicShareWithControl
{
  //inherit our parents constructor
  using ExecutionArrayInterfaceBasicShareWithControl::ExecutionArrayInterfaceBasicShareWithControl;

  VTKM_CONT
  DeviceAdapterId GetDeviceId() const final;
};

} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_tbb_internal_ExecutionArrayInterfaceBasicTBB_h
