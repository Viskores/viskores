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

#include <vtkm/cont/Field.h>
#include <vtkm/cont/CoordinateSystem.h>

namespace vtkm {
namespace filter {

class FieldMetadata
{
public:
  VTKM_CONT_EXPORT
  FieldMetadata():
    Name(),
    Association(vtkm::cont::Field::ASSOC_ANY),
    CellSetName()
    {
    }

  VTKM_CONT_EXPORT
  FieldMetadata(const vtkm::cont::Field& f):
    Name(f.GetName()),
    Association(f.GetAssociation()),
    CellSetName(f.GetAssocCellSet())
    {
    }

  VTKM_CONT_EXPORT
  FieldMetadata(const vtkm::cont::CoordinateSystem &sys):
    Name(sys.GetName()),
    Association(sys.GetAssociation()),
    CellSetName(sys.GetAssocCellSet())
    {
    }

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
    {return this->CellSetName; }

  template<typename T, typename StorageTag>
  VTKM_CONT_EXPORT
  vtkm::cont::Field AsField(const vtkm::cont::ArrayHandle<T,StorageTag> &handle) const
    {
      //Field only handles arrayHandles with default storage tag, so use
      //dynamic array handles
      vtkm::cont::DynamicArrayHandle dhandle(handle);
      if(this->IsCellField())
      {
        return vtkm::cont::Field(this->Name, this->Association,
                                 this->CellSetName, dhandle);
      }
      else
      {
        return vtkm::cont::Field(this->Name, this->Association, dhandle);
      }
    }

private:
  std::string         Name;  ///< name of field
  vtkm::cont::Field::AssociationEnum   Association;
  std::string         CellSetName;  ///< only populate if assoc is cells
};

}
}


#endif //vtk_m_filter_FieldMetadata_h
