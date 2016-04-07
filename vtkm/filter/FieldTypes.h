//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_filter_FieldMetadata_h
#define vtk_m_filter_FieldMetadata_h

#include <vtkm/filter/PolicyBase.h>

namespace vtkm {
namespace filter {

class FieldMetadata
{
public:
  VTKM_CONT_EXPORT
  FieldMetadata(const vtkm::cont::Field& f):
    Name(f.GetName()),
    Association(f.GetAssociation()),
    Order(f.GetOrder()),
    CellSetName(f.GetAssocCellSet()),
    LogicalDim(f.GetAssocLogicalDim()),
    {
    }

  VTKM_CONT_EXPORT
  FieldMetadata(const vtkm::cont::CoordinateSystem &sys):
    Name(f.GetName()),
    Association(f.GetAssociation()),
    Order(f.GetOrder()),
    CellSetName(f.GetAssocCellSet()),
    LogicalDim(f.GetAssocLogicalDim()),
    {
    }

  VTKM_CONT_EXPORT
  bool operator==(const FieldMetadata& other) const
  { return this->Association == other.Association; }

  VTKM_CONT_EXPORT
  bool operator!=(const FieldMetadata& other) const
  { return this->Association != other.Association; }

  VTKM_CONT_EXPORT
  bool IsPointField() const
    {return this->Association == vtkm::cont::Field::ASSOC_POINTS; }

  VTKM_CONT_EXPORT
  bool IsCellField() const
    {return this->Association == vtkm::cont::Field::ASSOC_CELL_SET; }

  VTKM_CONT_EXPORT
  const std::string& GetName() const
    {return this->Name; }

  VTKM_CONT_EXPORT
  const std::string& GetCellSetName() const
    {return this->AssocCellSetName; }

private:
  std::string         Name;  ///< name of field
  vtkm::cont::Field::AssociationEnum   Association;
  vtkm::IdComponent   Order; ///< 0=(piecewise) constant, 1=linear, 2=quadratic
  std::string         AssocCellSetName;  ///< only populate if assoc is cells
  vtkm::IdComponent   AssocLogicalDim; ///< only populate if assoc is logical dim
};

}
}


#endif //vtk_m_filter_FieldMetadata_h
