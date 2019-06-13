//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
    if (this->IsCellField())
    {
      return vtkm::cont::Field(this->Name, this->Association, this->CellSetName, handle);
    }
    else
    {
      return vtkm::cont::Field(this->Name, this->Association, handle);
    }
  }

  VTKM_CONT
  vtkm::cont::Field AsField(const vtkm::cont::VariantArrayHandle& handle) const
  {
    if (this->IsCellField())
    {
      return vtkm::cont::Field(this->Name, this->Association, this->CellSetName, handle);
    }
    else
    {
      return vtkm::cont::Field(this->Name, this->Association, handle);
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
