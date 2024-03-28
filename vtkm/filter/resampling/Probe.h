//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_resampling_Probe_h
#define vtk_m_filter_resampling_Probe_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/resampling/vtkm_filter_resampling_export.h>

namespace vtkm
{
namespace filter
{
namespace resampling
{

/// @brief Sample the fields of a data set at specified locations.
///
/// The `vtkm::filter::resampling::Probe` filter samples the fields of one
/// `vtkm::cont::DataSet` and places them in the fields of another
/// `vtkm::cont::DataSet`.
///
/// To use this filter, first specify a geometry to probe with with `SetGeometry()`.
/// The most important feature of this geometry is its coordinate system.
/// When you call `Execute()`, the output will be the data specified with
/// `SetGeometry()` but will have the fields of the input to `Execute()`
/// transferred to it. The fields are transfered by probing the input data
/// set at the point locations of the geometry.
///
class VTKM_FILTER_RESAMPLING_EXPORT Probe : public vtkm::filter::Filter
{
public:
  /// @brief Specify the geometry to probe with.
  ///
  /// When `Execute()` is called, the input data will be probed at all the point
  /// locations of this @p geometry as specified by its coordinate system.
  VTKM_CONT void SetGeometry(const vtkm::cont::DataSet& geometry)
  {
    this->Geometry = vtkm::cont::DataSet();
    this->Geometry.CopyStructure(geometry);
  }

  /// @copydoc SetGeometry
  VTKM_CONT const vtkm::cont::DataSet& GetGeometry() const { return this->Geometry; }

  /// @brief Specify the value to use for points outside the bounds of the input.
  ///
  /// It is possible that the sampling geometry will have points outside the bounds of
  /// the input. When this happens, the field will be set to this "invalid" value.
  /// By default, the invalid value is NaN.
  VTKM_CONT void SetInvalidValue(vtkm::Float64 invalidValue) { this->InvalidValue = invalidValue; }
  /// @copydoc SetInvalidValue
  VTKM_CONT vtkm::Float64 GetInvalidValue() const { return this->InvalidValue; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::cont::DataSet Geometry;

  vtkm::Float64 InvalidValue = vtkm::Nan64();
};

} // namespace resampling
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_resampling_Probe_h
