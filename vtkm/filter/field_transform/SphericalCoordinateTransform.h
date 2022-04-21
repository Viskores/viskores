//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_transform_SphericalCoordinateTransform_h
#define vtk_m_filter_field_transform_SphericalCoordinateTransform_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{

class VTKM_FILTER_FIELD_TRANSFORM_EXPORT SphericalCoordinateTransform
  : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT void SetCartesianToSpherical() { CartesianToSpherical = true; }
  VTKM_CONT void SetSphericalToCartesian() { CartesianToSpherical = false; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  bool CartesianToSpherical = true;
};
} // namespace field_transform
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::field_transform::SphericalCoordinateTransform.")
  SphericalCoordinateTransform : public vtkm::filter::field_transform::SphericalCoordinateTransform
{
  using field_transform::SphericalCoordinateTransform::SphericalCoordinateTransform;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_transform_SphericalCoordinateTransform_h
