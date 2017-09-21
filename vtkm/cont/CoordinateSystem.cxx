//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/CoordinateSystem.h>

namespace vtkm
{
namespace cont
{

VTKM_CONT
void CoordinateSystem::PrintSummary(std::ostream& out) const
{
  out << "    Coordinate System ";
  this->Superclass::PrintSummary(out);
}

VTKM_CONT
void CoordinateSystem::GetRange(vtkm::Range* range) const
{
  this->Superclass::GetRange(range,
                             VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG(),
                             VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
}

VTKM_CONT
const vtkm::cont::ArrayHandle<vtkm::Range>& CoordinateSystem::GetRange() const
{
  return this->Superclass::GetRange(VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG(),
                                    VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
}

VTKM_CONT
vtkm::Bounds CoordinateSystem::GetBounds() const
{
  return this->GetBounds(VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG(),
                         VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
}
}
} // namespace vtkm::cont
