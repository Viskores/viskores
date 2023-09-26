//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_transform_WarpVector_h
#define vtk_m_filter_field_transform_WarpVector_h

#include <vtkm/filter/field_transform/Warp.h>

#include <vtkm/Deprecated.h>

struct VTKM_DEPRECATED(2.2, "WarpVector.h header no longer supported. Use Warp.h.")
  vtkm_deprecated_WarpVector_h_warning
{
};

vtkm_deprecated_WarpVector_h_warning vtkm_give_WarpVector_h_warning;

namespace vtkm
{
namespace filter
{
namespace field_transform
{

class VTKM_DEPRECATED(2.2, "Use more general Warp filter.") WarpVector
  : public vtkm::filter::field_transform::Warp
{
public:
  VTKM_DEPRECATED(2.2, "Use SetScaleFactor().")
  VTKM_CONT explicit WarpVector(vtkm::FloatDefault scale)
  {
    this->SetScaleFactor(scale);
    this->SetOutputFieldName("warpvector");
  }

  VTKM_DEPRECATED(2.2, "Use SetDirectionField().")
  VTKM_CONT void SetVectorField(
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
  VTKM_CONT std::string GetVectorFieldName() const { return this->GetDirectionFieldName(); }

  VTKM_DEPRECATED(2.2, "Only point association supported.")
  VTKM_CONT vtkm::cont::Field::Association GetVectorFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }
};

} // namespace field_transform
} // namespace filter
} // namespace vtkm
#endif // vtk_m_filter_field_transform_WarpVector_h
