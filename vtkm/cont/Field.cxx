//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace cont
{

/// constructors for points / whole mesh
VTKM_CONT
Field::Field(std::string name, Association association, const vtkm::cont::VariantArrayHandle& data)
  : Name(name)
  , FieldAssociation(association)
  , Data(data)
  , Range()
  , ModifiedFlag(true)
{
}

VTKM_CONT
Field::Field(const vtkm::cont::Field& src)
  : Name(src.Name)
  , FieldAssociation(src.FieldAssociation)
  , Data(src.Data)
  , Range(src.Range)
  , ModifiedFlag(src.ModifiedFlag)
{
}

VTKM_CONT
Field::Field(vtkm::cont::Field&& src) noexcept : Name(std::move(src.Name)),
                                                 FieldAssociation(std::move(src.FieldAssociation)),
                                                 Data(std::move(src.Data)),
                                                 Range(std::move(src.Range)),
                                                 ModifiedFlag(std::move(src.ModifiedFlag))
{
}

VTKM_CONT
Field& Field::operator=(const vtkm::cont::Field& src)
{
  this->Name = src.Name;
  this->FieldAssociation = src.FieldAssociation;
  this->Data = src.Data;
  this->Range = src.Range;
  this->ModifiedFlag = src.ModifiedFlag;
  return *this;
}

VTKM_CONT
Field& Field::operator=(vtkm::cont::Field&& src) noexcept
{
  this->Name = std::move(src.Name);
  this->FieldAssociation = std::move(src.FieldAssociation);
  this->Data = std::move(src.Data);
  this->Range = std::move(src.Range);
  this->ModifiedFlag = std::move(src.ModifiedFlag);
  return *this;
}


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
  }
  this->Data.PrintSummary(out);
}

VTKM_CONT
Field::~Field()
{
}


VTKM_CONT
const vtkm::cont::VariantArrayHandle& Field::GetData() const
{
  return this->Data;
}

VTKM_CONT
vtkm::cont::VariantArrayHandle& Field::GetData()
{
  this->ModifiedFlag = true;
  return this->Data;
}
}
} // namespace vtkm::cont
