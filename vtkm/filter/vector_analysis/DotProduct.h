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

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/vector_analysis/vtkm_filter_vector_analysis_export.h>

namespace vtkm
{
namespace filter
{
namespace vector_analysis
{

/// @brief Compute the dot product of vector fields.
///
/// The left part of the operand is the "primary" field and the right part of the operand
/// is the "secondary" field (although the dot product is commutative, so the order of
/// primary and secondary seldom matters).
///
/// The dot product can operate on vectors of any length.
class VTKM_FILTER_VECTOR_ANALYSIS_EXPORT DotProduct : public vtkm::filter::Filter
{
public:
  VTKM_CONT DotProduct();

  /// @brief Specify the primary field to operate on.
  ///
  /// In the dot product operation A . B, A is the primary field.
  ///
  /// The primary field is an alias for active field index 0. As with any active field,
  /// it can be set as a named field or as a coordinate system.
  VTKM_CONT void SetPrimaryField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    this->SetActiveField(name, association);
  }

  /// @copydoc SetPrimaryField
  VTKM_CONT const std::string& GetPrimaryFieldName() const { return this->GetActiveFieldName(); }
  /// @copydoc SetPrimaryField
  VTKM_CONT vtkm::cont::Field::Association GetPrimaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation();
  }

  /// @copydoc SetPrimaryField
  VTKM_CONT void SetUseCoordinateSystemAsPrimaryField(bool flag)
  {
    this->SetUseCoordinateSystemAsField(flag);
  }
  /// @copydoc SetPrimaryField
  VTKM_CONT bool GetUseCoordinateSystemAsPrimaryField() const
  {
    return this->GetUseCoordinateSystemAsField();
  }

  /// @copydoc SetPrimaryField
  VTKM_CONT void SetPrimaryCoordinateSystem(vtkm::Id coord_idx)
  {
    this->SetActiveCoordinateSystem(coord_idx);
  }
  /// @copydoc SetPrimaryField
  VTKM_CONT vtkm::Id GetPrimaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex();
  }

  /// @brief Specify the secondary field to operate on.
  ///
  /// In the dot product operation A . B, B is the secondary field.
  ///
  /// The secondary field is an alias for active field index 1. As with any active field,
  /// it can be set as a named field or as a coordinate system.
  VTKM_CONT void SetSecondaryField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    this->SetActiveField(1, name, association);
  }

  /// @copydoc SetSecondaryField
  VTKM_CONT const std::string& GetSecondaryFieldName() const { return this->GetActiveFieldName(1); }
  /// @copydoc SetSecondaryField
  VTKM_CONT vtkm::cont::Field::Association GetSecondaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }

  /// @copydoc SetSecondaryField
  VTKM_CONT void SetUseCoordinateSystemAsSecondaryField(bool flag)
  {
    this->SetUseCoordinateSystemAsField(1, flag);
  }
  /// @copydoc SetSecondaryField
  VTKM_CONT bool GetUseCoordinateSystemAsSecondaryField() const
  {
    return this->GetUseCoordinateSystemAsField(1);
  }

  /// @copydoc SetSecondaryField
  VTKM_CONT void SetSecondaryCoordinateSystem(vtkm::Id index)
  {
    this->SetActiveCoordinateSystem(1, index);
  }
  /// @copydoc SetSecondaryField
  VTKM_CONT vtkm::Id GetSecondaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex(1);
  }

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

} // namespace vector_analysis
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_vector_analysis_DotProduct_h
