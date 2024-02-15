//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_entity_extraction_Threshold_h
#define vtk_m_filter_entity_extraction_Threshold_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{

/// \brief Extracts cells that satisfy a threshold criterion.
///
/// Extracts all cells from any dataset type that satisfy a threshold criterion.
/// The output of this filter stores its connectivity in a `vtkm::cont::CellSetExplicit<>`
/// regardless of the input dataset type or which cells are passed.
///
/// You can threshold either on point or cell fields. If thresholding on point fields,
/// you must specify whether a cell should be kept if some but not all of its incident
/// points meet the criteria.
///
/// Although `Threshold` is primarily designed for scalar fields, there is support for
/// thresholding on 1 or all of the components in a vector field. See the
/// `SetComponentToTest()`, `SetComponentToTestToAny()`, and `SetComponentToTestToAll()`
/// methods for more information.
///
/// Use `SetActiveField()` and related methods to set the field to threshold on.
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT Threshold : public vtkm::filter::Filter
{
public:
  /// @brief Specifies the lower scalar value.
  /// Any cells where the scalar field is less than this value are removed.
  VTKM_CONT void SetLowerThreshold(vtkm::Float64 value) { this->LowerValue = value; }
  /// @brief Specifies the upper scalar value.
  /// Any cells where the scalar field is more than this value are removed.
  VTKM_CONT void SetUpperThreshold(vtkm::Float64 value) { this->UpperValue = value; }

  /// @copydoc SetLowerThreshold
  VTKM_CONT vtkm::Float64 GetLowerThreshold() const { return this->LowerValue; }
  /// @copydoc SetUpperThreshold
  VTKM_CONT vtkm::Float64 GetUpperThreshold() const { return this->UpperValue; }

  /// @brief  Sets the threshold criterion to pass any value less than or equal to @a value.
  VTKM_CONT void SetThresholdBelow(vtkm::Float64 value);

  /// @brief Sets the threshold criterion to pass any value greater than or equal to @a value.
  VTKM_CONT void SetThresholdAbove(vtkm::Float64 value);

  /// @brief Set the threshold criterion to pass any value between (inclusive) the given values.
  ///
  /// This method is equivalent to calling `SetLowerThreshold(value1)` and
  /// `SetUpperThreshold(value2)`.
  VTKM_CONT void SetThresholdBetween(vtkm::Float64 value1, vtkm::Float64 value2);

  /// @brief Specifies that the threshold criteria should be applied to a specific vector component.
  ///
  /// When thresholding on a vector field (which has more than one component per entry),
  /// the `Threshold` filter will by default compare the threshold criterion to the first
  /// component of the vector (component index 0). Use this method to change the component
  /// to test against.
  VTKM_CONT void SetComponentToTest(vtkm::IdComponent component)
  {
    this->ComponentMode = Component::Selected;
    this->SelectedComponent = component;
  }
  /// @brief Specifies that the threshold criteria should be applied to a specific vector component.
  ///
  /// This method sets that the threshold criteria should be applied to all the components of
  /// the input vector field and a cell will pass if @e any the components match.
  VTKM_CONT void SetComponentToTestToAny() { this->ComponentMode = Component::Any; }
  /// @brief Specifies that the threshold criteria should be applied to a specific vector component.
  ///
  /// This method sets that the threshold criteria should be applied to all the components of
  /// the input vector field and a cell will pass if @e all the components match.
  VTKM_CONT void SetComponentToTestToAll() { this->ComponentMode = Component::All; }

  /// @brief Specify criteria for cells that have some points matching.
  ///
  /// When thresholding on a point field, each cell must consider the multiple values
  /// associated with all incident points. When this flag is false (the default), the
  /// cell is passed if @e any of the incident points matches the threshold criterion.
  /// When this flag is true, the cell is passed only if \e all the incident points match
  /// the threshold criterion.
  VTKM_CONT void SetAllInRange(bool value) { this->AllInRange = value; }
  /// @copydoc SetAllInRange
  VTKM_CONT bool GetAllInRange() const { return this->AllInRange; }

  /// @brief Inverts the threshold result.
  ///
  /// When set to true, the threshold result is inverted. That is, cells that would have been
  /// in the output with this option set to false (the default) are excluded while cells that
  /// would have been excluded from the output are included.
  VTKM_CONT void SetInvert(bool value) { this->Invert = value; }
  /// @copydoc SetInvert
  VTKM_CONT bool GetInvert() const { return this->Invert; }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  double LowerValue = 0;
  double UpperValue = 0;

  enum struct Component
  {
    Any,
    All,
    Selected
  };

  Component ComponentMode = Component::Selected;
  vtkm::IdComponent SelectedComponent = 0;

  bool AllInRange = false;
  bool Invert = false;
};
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_entity_extraction_Threshold_h
