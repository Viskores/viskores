//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_vector_calculus_DotProduct_h
#define vtk_m_filter_vector_calculus_DotProduct_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/vector_calculus/vtkm_filter_vector_calculus_export.h>

namespace vtkm
{
namespace filter
{
namespace vector_calculus
{
class VTKM_FILTER_VECTOR_CALCULUS_EXPORT DotProduct : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT DotProduct();

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
    this->SetActiveField(1, name, association);
  }

  VTKM_CONT const std::string& GetSecondaryFieldName() const { return this->GetActiveFieldName(1); }
  VTKM_CONT vtkm::cont::Field::Association GetSecondaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }
  //@}

  //@{
  /// When set to true, uses a coordinate system as the primary field instead of the one selected
  /// by name. Use SetPrimaryCoordinateSystem to select which coordinate system.
  VTKM_CONT
  void SetUseCoordinateSystemAsSecondaryField(bool flag)
  {
    this->SetUseCoordinateSystemAsField(1, flag);
  }
  VTKM_CONT
  bool GetUseCoordinateSystemAsSecondaryField() const
  {
    return this->GetUseCoordinateSystemAsField(1);
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

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Id SecondaryCoordinateSystemIndex = 0;
};
} // namespace vector_calculus
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_vector_calculus_DotProduct_h
