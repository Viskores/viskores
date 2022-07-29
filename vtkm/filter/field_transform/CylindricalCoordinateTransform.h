//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_transform_CylindricalCoordinateTransform_h
#define vtk_m_filter_field_transform_CylindricalCoordinateTransform_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
/// \brief
///
/// Generate a coordinate transformation on coordinates from a dataset.
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT CylindricalCoordinateTransform
  : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT void SetCartesianToCylindrical() { CartesianToCylindrical = true; }
  VTKM_CONT void SetCylindricalToCartesian() { CartesianToCylindrical = false; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  bool CartesianToCylindrical = true;
};
} // namespace field_transform
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::field_transform::CylindricalCoordinateTransform.")
  CylindricalCoordinateTransform
  : public vtkm::filter::field_transform::CylindricalCoordinateTransform
{
  using field_transform::CylindricalCoordinateTransform::CylindricalCoordinateTransform;
};
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_field_transform_CylindricalCoordinateTransform_h
