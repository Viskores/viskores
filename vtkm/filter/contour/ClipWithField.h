//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_contour_ClipWithField_h
#define vtk_m_filter_contour_ClipWithField_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
/// \brief Clip a dataset using a field
///
/// Clip a dataset using a given field value. All points that are less than that
/// value are considered outside, and will be discarded. All points that are greater
/// are kept.
///
/// To select the scalar field, use the `SetActiveField()` and related methods.
///
class VTKM_FILTER_CONTOUR_EXPORT ClipWithField : public vtkm::filter::Filter
{
public:
  /// @brief Specifies the field value for the clip operation.
  ///
  /// Regions where the active field is less than this value are clipped away
  /// from each input cell.
  VTKM_CONT void SetClipValue(vtkm::Float64 value) { this->ClipValue = value; }

  /// @brief Specifies if the result for the clip filter should be inverted.
  ///
  /// If set to false (the default), regions where the active field is less than
  /// the specified clip value are removed. If set to true, regions where the active
  /// field is more than the specified clip value are removed.
  VTKM_CONT void SetInvertClip(bool invert) { this->Invert = invert; }

  /// @brief Specifies the field value for the clip operation.
  VTKM_CONT vtkm::Float64 GetClipValue() const { return this->ClipValue; }

  /// @brief Specifies if the result for the clip filter should be inverted.
  VTKM_CONT bool GetInvertClip() const { return this->Invert; }

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Float64 ClipValue = 0;
  bool Invert = false;
};
} // namespace contour
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_ClipWithField_h
