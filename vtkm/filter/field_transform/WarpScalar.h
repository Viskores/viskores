//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_transform_WarpScalar_h
#define vtk_m_filter_field_transform_WarpScalar_h

#include <vtkm/filter/field_transform/Warp.h>

#include <vtkm/Deprecated.h>

struct VTKM_DEPRECATED(2.2, "WarpScalar.h header no longer supported. Use Warp.h.")
  vtkm_deprecated_WarpScalar_h_warning
{
};

vtkm_deprecated_WarpScalar_h_warning vtkm_give_WarpScalar_h_warning;

namespace vtkm
{
namespace filter
{
namespace field_transform
{

class VTKM_DEPRECATED(2.2, "Use more general Warp filter.") WarpScalar
  : public vtkm::filter::field_transform::Warp
{
public:
  VTKM_DEPRECATED(2.2, "Use SetScaleFactor().")
  VTKM_CONT explicit WarpScalar(vtkm::FloatDefault scaleAmount)
  {
    this->SetScaleFactor(scaleAmount);
    this->SetOutputFieldName("warpscalar");
  }

  VTKM_DEPRECATED(2.2, "Use SetDirectionField().")
  VTKM_CONT void SetNormalField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    if ((association != vtkm::cont::Field::Association::Any) &&
        (association != vtkm::cont::Field::Association::Points))
    {
      throw vtkm::cont::ErrorBadValue("Normal field should always be associated with points.");
    }
    this->SetDirectionField(name);
  }

  VTKM_DEPRECATED(2.2, "Use GetDirectionFieldName().")
  VTKM_CONT std::string GetNormalFieldName() const { return this->GetDirectionFieldName(); }

  VTKM_DEPRECATED(2.2, "Only point association supported.")
  VTKM_CONT vtkm::cont::Field::Association GetNormalFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }

  VTKM_DEPRECATED(2.2, "Use SetScaleField().")
  VTKM_CONT void SetScalarFactorField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    if ((association != vtkm::cont::Field::Association::Any) &&
        (association != vtkm::cont::Field::Association::Points))
    {
      throw vtkm::cont::ErrorBadValue("Normal field should always be associated with points.");
    }
    this->SetScaleField(name);
  }

  VTKM_DEPRECATED(2.2, "Use GetScaleField().")
  VTKM_CONT std::string GetScalarFactorFieldName() const { return this->GetScaleFieldName(); }

  VTKM_DEPRECATED(2.2, "Only point association supported.")
  VTKM_CONT vtkm::cont::Field::Association GetScalarFactorFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }
};

} // namespace field_transform
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_field_transform_WarpScalar_h
