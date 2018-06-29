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

#include <vtkm/cont/BoundingIntervalHierarchy.h>
#include <vtkm/cont/BoundingIntervalHierarchy.hxx>

namespace vtkm
{
namespace cont
{

VTKM_CONT
void BoundingIntervalHierarchy::Build()
{
  BuildFunctor functor(this);
  vtkm::cont::TryExecute(functor);
}

VTKM_CONT
const vtkm::exec::CellLocator* BoundingIntervalHierarchy::PrepareForExecutionImpl(
  const vtkm::Int8 device) const
{
  using DeviceList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG;
  const vtkm::exec::CellLocator* toReturn;
  vtkm::cont::internal::FindDeviceAdapterTagAndCall(
    device, DeviceList(), PrepareForExecutionFunctor(), this, &toReturn);
  return toReturn;
}
}
}
