//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_vector_analysis_DotProduct_h
#define vtk_m_filter_vector_analysis_DotProduct_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/vector_analysis/vtkm_filter_vector_analysis_export.h>

namespace vtkm
{
namespace filter
{
namespace vector_analysis
{
class VTKM_FILTER_VECTOR_ANALYSIS_EXPORT DotProduct : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT DotProduct();

  //@{
  /// Choose the primary field to operate on. In the dot product operation A . B, A is
  /// the primary field.
  VTKM_CONT
  void SetPrimaryField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
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
  /// Select the coordinate system coord_idx to use as the primary field. This only has an effect when
  /// UseCoordinateSystemAsPrimaryField is true.
  VTKM_CONT
  void SetPrimaryCoordinateSystem(vtkm::Id coord_idx)
  {
    this->SetActiveCoordinateSystem(coord_idx);
  }
  VTKM_CONT
  vtkm::Id GetPrimaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex();
  }
  //@}

  //@{
  /// Choose the secondary field to operate on. In the dot product operation A . B, B is
  /// the secondary field.
  VTKM_CONT
  void SetSecondaryField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
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
  /// When set to true, uses a coordinate system as the secondary field instead of the one selected
  /// by name. Use SetSecondaryCoordinateSystem to select which coordinate system.
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
  /// Select the coordinate system index to use as the secondary field. This only has an effect when
  /// UseCoordinateSystemAsSecondaryField is true.
  VTKM_CONT
  void SetSecondaryCoordinateSystem(vtkm::Id index) { this->SetActiveCoordinateSystem(1, index); }
  VTKM_CONT
  vtkm::Id GetSecondaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex(1);
  }
  //@}

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace vector_analysis
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::vector_analysis::DotProduct.") DotProduct
  : public vtkm::filter::vector_analysis::DotProduct
{
  using vector_analysis::DotProduct::DotProduct;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_vector_analysis_DotProduct_h
