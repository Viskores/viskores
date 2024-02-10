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

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{

/// @brief Transform coordinates between Cartesian and spherical.
///
/// By default, this filter will transform the first coordinate system, but
/// this can be changed by setting the active field.
///
/// The resulting transformation will be set as the first coordinate system
/// in the output.
///
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT SphericalCoordinateTransform : public vtkm::filter::Filter
{
public:
  VTKM_CONT SphericalCoordinateTransform();

  /// @brief Establish a transformation from Cartesian to spherical coordinates.
  VTKM_CONT void SetCartesianToSpherical() { CartesianToSpherical = true; }
  /// @brief Establish a transformation from spherical to Cartesian coordiantes.
  VTKM_CONT void SetSphericalToCartesian() { CartesianToSpherical = false; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  bool CartesianToSpherical = true;
};
} // namespace field_transform
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_transform_SphericalCoordinateTransform_h
