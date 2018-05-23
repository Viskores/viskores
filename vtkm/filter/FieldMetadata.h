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

#ifndef vtk_m_filter_FieldMetadata_h
#define vtk_m_filter_FieldMetadata_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace filter
{

class FieldMetadata
{
public:
  VTKM_CONT
  FieldMetadata()
    : Name()
    , Association(vtkm::cont::Field::Association::ANY)
    , CellSetName()
  {
  }

  VTKM_CONT
  FieldMetadata(const vtkm::cont::Field& f)
    : Name(f.GetName())
    , Association(f.GetAssociation())
    , CellSetName(f.GetAssocCellSet())
  {
  }

  VTKM_CONT
  FieldMetadata(const vtkm::cont::CoordinateSystem& sys)
    : Name(sys.GetName())
    , Association(sys.GetAssociation())
    , CellSetName(sys.GetAssocCellSet())
  {
  }

  VTKM_CONT
  bool IsPointField() const { return this->Association == vtkm::cont::Field::Association::POINTS; }

  VTKM_CONT
  bool IsCellField() const { return this->Association == vtkm::cont::Field::Association::CELL_SET; }

  VTKM_CONT
  const std::string& GetName() const { return this->Name; }

  VTKM_CONT
  vtkm::cont::Field::Association GetAssociation() const { return this->Association; }

  VTKM_CONT
  const std::string& GetCellSetName() const { return this->CellSetName; }

  template <typename T, typename StorageTag>
  VTKM_CONT vtkm::cont::Field AsField(const vtkm::cont::ArrayHandle<T, StorageTag>& handle) const
  {
    //Field only handles arrayHandles with default storage tag, so use
    //dynamic array handles
    vtkm::cont::DynamicArrayHandle dhandle(handle);
    if (this->IsCellField())
    {
      return vtkm::cont::Field(this->Name, this->Association, this->CellSetName, dhandle);
    }
    else
    {
      return vtkm::cont::Field(this->Name, this->Association, dhandle);
    }
  }

  VTKM_CONT
  vtkm::cont::Field AsField(const vtkm::cont::DynamicArrayHandle& dhandle) const
  {
    if (this->IsCellField())
    {
      return vtkm::cont::Field(this->Name, this->Association, this->CellSetName, dhandle);
    }
    else
    {
      return vtkm::cont::Field(this->Name, this->Association, dhandle);
    }
  }

private:
  std::string Name; ///< name of field
  vtkm::cont::Field::Association Association;
  std::string CellSetName; ///< only populate if assoc is cells
};
}
}

#endif //vtk_m_filter_FieldMetadata_h
