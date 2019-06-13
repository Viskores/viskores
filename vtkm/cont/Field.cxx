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
  , AssocCellSetName()
  , AssocLogicalDim(-1)
  , Data(data)
  , Range()
  , ModifiedFlag(true)
{
  VTKM_ASSERT(this->FieldAssociation == Association::WHOLE_MESH ||
              this->FieldAssociation == Association::POINTS);
}

/// constructors for cell set associations
VTKM_CONT
Field::Field(std::string name,
             Association association,
             const std::string& cellSetName,
             const vtkm::cont::VariantArrayHandle& data)
  : Name(name)
  , FieldAssociation(association)
  , AssocCellSetName(cellSetName)
  , AssocLogicalDim(-1)
  , Data(data)
  , Range()
  , ModifiedFlag(true)
{
  VTKM_ASSERT(this->FieldAssociation == Association::CELL_SET);
}

/// constructors for logical dimension associations
VTKM_CONT
Field::Field(std::string name,
             Association association,
             vtkm::IdComponent logicalDim,
             const vtkm::cont::VariantArrayHandle& data)
  : Name(name)
  , FieldAssociation(association)
  , AssocCellSetName()
  , AssocLogicalDim(logicalDim)
  , Data(data)
  , Range()
  , ModifiedFlag(true)
{
  VTKM_ASSERT(this->FieldAssociation == Association::LOGICAL_DIM);
}

VTKM_CONT
Field::Field(const vtkm::cont::Field& src)
  : Name(src.Name)
  , FieldAssociation(src.FieldAssociation)
  , AssocCellSetName(src.AssocCellSetName)
  , AssocLogicalDim(src.AssocLogicalDim)
  , Data(src.Data)
  , Range(src.Range)
  , ModifiedFlag(src.ModifiedFlag)
{
}

VTKM_CONT
Field::Field(vtkm::cont::Field&& src) noexcept : Name(std::move(src.Name)),
                                                 FieldAssociation(std::move(src.FieldAssociation)),
                                                 AssocCellSetName(std::move(src.AssocCellSetName)),
                                                 AssocLogicalDim(std::move(src.AssocLogicalDim)),
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
  this->AssocCellSetName = src.AssocCellSetName;
  this->AssocLogicalDim = src.AssocLogicalDim;
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
  this->AssocCellSetName = std::move(src.AssocCellSetName);
  this->AssocLogicalDim = std::move(src.AssocLogicalDim);
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
