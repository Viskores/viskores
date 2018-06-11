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

#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace cont
{

VTKM_CONT
void Field::PrintSummary(std::ostream& out) const
{
  out << "   " << this->Name;
  out << " assoc= ";
  switch (this->GetAssociation())
  {
    case Association::ANY:
      out << "Any ";
      break;
    case Association::WHOLE_MESH:
      out << "Mesh ";
      break;
    case Association::POINTS:
      out << "Points ";
      break;
    case Association::CELL_SET:
      out << "Cells ";
      break;
    case Association::LOGICAL_DIM:
      out << "LogicalDim ";
      break;
  }
  this->Data.PrintSummary(out);
}

VTKM_CONT
Field::~Field()
{
}

VTKM_CONT
const vtkm::cont::ArrayHandle<vtkm::Range>& Field::GetRange() const
{
  return this->GetRange(VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}

VTKM_CONT
void Field::GetRange(vtkm::Range* range) const
{
  this->GetRange(range, VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}

VTKM_CONT
const vtkm::cont::DynamicArrayHandle& Field::GetData() const
{
  return this->Data;
}

VTKM_CONT
vtkm::cont::DynamicArrayHandle& Field::GetData()
{
  this->ModifiedFlag = true;
  return this->Data;
}

VTKM_CONT
const vtkm::cont::ArrayHandle<vtkm::Range>& Field::GetRange(VTKM_DEFAULT_TYPE_LIST_TAG,
                                                            VTKM_DEFAULT_STORAGE_LIST_TAG) const
{
  return this->GetRangeImpl(VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}
}
} // namespace vtkm::cont
