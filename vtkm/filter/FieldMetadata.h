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
  {
  }

  VTKM_CONT
  FieldMetadata(const vtkm::cont::Field& f)
    : Name(f.GetName())
    , Association(f.GetAssociation())
  {
  }

  VTKM_CONT
  FieldMetadata(const vtkm::cont::CoordinateSystem& sys)
    : Name(sys.GetName())
    , Association(sys.GetAssociation())
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

  /// Construct a new field with the same association as stored in this FieldMetaData
  /// but with a new name
  template <typename T, typename StorageTag>
  VTKM_CONT vtkm::cont::Field AsField(const std::string& name,
                                      const vtkm::cont::ArrayHandle<T, StorageTag>& handle) const
  {
    return vtkm::cont::Field(name, this->Association, handle);
  }
  /// Construct a new field with the same association as stored in this FieldMetaData
  /// but with a new name
  VTKM_CONT
  vtkm::cont::Field AsField(const std::string& name,
                            const vtkm::cont::VariantArrayHandle& handle) const
  {
    return vtkm::cont::Field(name, this->Association, handle);
  }

  /// Construct a new field with the same association and name as stored in this FieldMetaData
  template <typename T, typename StorageTag>
  VTKM_CONT vtkm::cont::Field AsField(const vtkm::cont::ArrayHandle<T, StorageTag>& handle) const
  {
    return this->AsField(this->Name, handle);
  }
  /// Construct a new field with the same association and name as stored in this FieldMetaData
  VTKM_CONT vtkm::cont::Field AsField(const vtkm::cont::VariantArrayHandle& handle) const
  {
    return this->AsField(this->Name, handle);
  }

private:
  std::string Name; ///< name of field
  vtkm::cont::Field::Association Association;
};
}
}

#endif //vtk_m_filter_FieldMetadata_h
