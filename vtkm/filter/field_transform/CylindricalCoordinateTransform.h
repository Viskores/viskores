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

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{

/// @brief Transform coordinates between Cartesian and cylindrical.
///
/// By default, this filter will transform the first coordinate system, but
/// this can be changed by setting the active field.
///
/// The resulting transformation will be set as the first coordinate system
/// in the output.
///
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT CylindricalCoordinateTransform
  : public vtkm::filter::Filter
{
public:
  VTKM_CONT CylindricalCoordinateTransform();

  /// @brief Establish a transformation from Cartesian to cylindrical coordinates.
  VTKM_CONT void SetCartesianToCylindrical() { CartesianToCylindrical = true; }
  /// @brief Establish a transformation from cylindrical to Cartesian coordiantes.
  VTKM_CONT void SetCylindricalToCartesian() { CartesianToCylindrical = false; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  bool CartesianToCylindrical = true;
};
} // namespace field_transform
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_field_transform_CylindricalCoordinateTransform_h
