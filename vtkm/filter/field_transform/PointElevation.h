//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_transform_PointElevation_h
#define vtk_m_filter_field_transform_PointElevation_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
/// \brief Generate a scalar field along a specified direction
///
/// The filter will take a data set and a field of 3 dimensional vectors and compute the
/// distance along a line defined by a low point and a high point. Any point in the plane
/// touching the low point and perpendicular to the line is set to the minimum range value
/// in the elevation whereas any point in the plane touching the high point and
/// perpendicular to the line is set to the maximum range value. All other values are
/// interpolated linearly between these two planes. This filter is commonly used to compute
/// the elevation of points in some direction, but can be repurposed for a variety of measures.
///
/// The default name for the output field is ``elevation'', but that can be
/// overridden as always using the `SetOutputFieldName()` method.
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT PointElevation : public vtkm::filter::Filter
{
public:
  VTKM_CONT PointElevation();

  /// @brief Specify the coordinate of the low point.
  ///
  /// The plane of low values is defined by the plane that contains the low point and
  /// is normal to the direction from the low point to the high point. All vector
  /// values on this plane are assigned the low value.
  VTKM_CONT void SetLowPoint(const vtkm::Vec3f_64& point) { this->LowPoint = point; }
  /// @copydoc SetLowPoint
  VTKM_CONT void SetLowPoint(vtkm::Float64 x, vtkm::Float64 y, vtkm::Float64 z)
  {
    this->SetLowPoint({ x, y, z });
  }

  /// @brief Specify the coordinate of the high point.
  ///
  /// The plane of high values is defined by the plane that contains the high point and
  /// is normal to the direction from the low point to the high point. All vector
  /// values on this plane are assigned the high value.
  VTKM_CONT void SetHighPoint(const vtkm::Vec3f_64& point) { this->HighPoint = point; }
  /// @copydoc SetHighPoint
  VTKM_CONT void SetHighPoint(vtkm::Float64 x, vtkm::Float64 y, vtkm::Float64 z)
  {
    this->SetHighPoint({ x, y, z });
  }

  /// @brief Specify the range of values to output.
  ///
  /// Values at the low plane are given @p low and values at the high plane are given
  /// @p high. Values in between the planes have a linearly interpolated value based
  /// on the relative distance between the two planes.
  VTKM_CONT void SetRange(vtkm::Float64 low, vtkm::Float64 high)
  {
    this->RangeLow = low;
    this->RangeHigh = high;
  }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Vec3f_64 LowPoint = { 0.0, 0.0, 0.0 };
  vtkm::Vec3f_64 HighPoint = { 0.0, 0.0, 1.0 };
  vtkm::Float64 RangeLow = 0.0, RangeHigh = 1.0;
};
} // namespace field_transform
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_transform_PointElevation_h
