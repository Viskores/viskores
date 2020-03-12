//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CrossProduct_h
#define vtk_m_filter_CrossProduct_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/CrossProduct.h>

namespace vtkm
{
namespace filter
{

class CrossProduct : public vtkm::filter::FilterField<CrossProduct>
{
public:
  //currently the DotProduct filter only works on vector data.
  using SupportedTypes = TypeListVecCommon;

  VTKM_CONT
  CrossProduct();

  //@{
  /// Choose the primary field to operate on. In the cross product operation A x B, A is
  /// the primary field.
  VTKM_CONT
  void SetPrimaryField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->SetActiveField(name, association);
  }

  VTKM_CONT const std::string& GetPrimaryFieldName() const { return this->GetActiveFieldName(); }
  VTKM_CONT vtkm::cont::Field::Association GetPrimaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation();
  }
  //@}

  //@{
  /// When set to true, uses a coordinate system as the primary field instead of the one selected
  /// by name. Use SetPrimaryCoordinateSystem to select which coordinate system.
  VTKM_CONT
  void SetUseCoordinateSystemAsPrimaryField(bool flag)
  {
    this->SetUseCoordinateSystemAsField(flag);
  }
  VTKM_CONT
  bool GetUseCoordinateSystemAsPrimaryField() const
  {
    return this->GetUseCoordinateSystemAsField();
  }
  //@}

  //@{
  /// Select the coordinate system index to use as the primary field. This only has an effect when
  /// UseCoordinateSystemAsPrimaryField is true.
  VTKM_CONT
  void SetPrimaryCoordinateSystem(vtkm::Id index) { this->SetActiveCoordinateSystem(index); }
  VTKM_CONT
  vtkm::Id GetPrimaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex();
  }
  //@}

  //@{
  /// Choose the secondary field to operate on. In the cross product operation A x B, B is
  /// the secondary field.
  VTKM_CONT
  void SetSecondaryField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->SecondaryFieldName = name;
    this->SecondaryFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetSecondaryFieldName() const { return this->SecondaryFieldName; }
  VTKM_CONT vtkm::cont::Field::Association GetSecondaryFieldAssociation() const
  {
    return this->SecondaryFieldAssociation;
  }
  //@}

  //@{
  /// When set to true, uses a coordinate system as the primary field instead of the one selected
  /// by name. Use SetPrimaryCoordinateSystem to select which coordinate system.
  VTKM_CONT
  void SetUseCoordinateSystemAsSecondaryField(bool flag)
  {
    this->UseCoordinateSystemAsSecondaryField = flag;
  }
  VTKM_CONT
  bool GetUseCoordinateSystemAsSecondaryField() const
  {
    return this->UseCoordinateSystemAsSecondaryField;
  }
  //@}

  //@{
  /// Select the coordinate system index to use as the primary field. This only has an effect when
  /// UseCoordinateSystemAsPrimaryField is true.
  VTKM_CONT
  void SetSecondaryCoordinateSystem(vtkm::Id index)
  {
    this->SecondaryCoordinateSystemIndex = index;
  }
  VTKM_CONT
  vtkm::Id GetSecondaryCoordinateSystemIndex() const
  {
    return this->SecondaryCoordinateSystemIndex;
  }
  //@}

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  std::string SecondaryFieldName;
  vtkm::cont::Field::Association SecondaryFieldAssociation;
  bool UseCoordinateSystemAsSecondaryField;
  vtkm::Id SecondaryCoordinateSystemIndex;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/CrossProduct.hxx>

#endif // vtk_m_filter_CrossProduct_h
